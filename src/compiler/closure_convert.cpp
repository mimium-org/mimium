#include "compiler/closure_convert.hpp"

namespace mimium {
ClosureConverter::ClosureConverter(TypeEnv& _typeenv)
    : typeenv(_typeenv), capturecount(0), tmp_globalfn("tmp", {}) {}

void ClosureConverter::reset() { capturecount = 0; }
ClosureConverter::~ClosureConverter() = default;

bool ClosureConverter::isKnownFunction(const std::string& name) {
  return known_functions.find(name) != known_functions.end();
}

void ClosureConverter::moveFunToTop(std::shared_ptr<MIRblock> mir) {
  for (auto& cinst : mir->instructions) {
    if (std::holds_alternative<std::shared_ptr<FunInst>>(cinst)) {
      auto& f = std::get<std::shared_ptr<FunInst>>(cinst);
      auto& tinsts = toplevel->instructions;
      moveFunToTop(f->body);
      if (this->toplevel != mir) {
        tinsts.insert(tinsts.begin(), f);  // move on top op toplevel
      }
      f->body->instructions.remove_if([](Instructions v) {
        return std::visit([](auto v) -> bool { return v->isFunction(); }, v);
      });
    }
  }
}

std::shared_ptr<MIRblock> ClosureConverter::convert(
    std::shared_ptr<MIRblock> toplevel) {
  // convert top level
  this->toplevel = toplevel;
  auto& inss = toplevel->instructions;
  for (auto it = inss.begin(), end = inss.end(); it != end; ++it) {
    auto& cinst = *it;
    if (std::holds_alternative<std::shared_ptr<FunInst>>(cinst)) {
      auto& f = std::get<std::shared_ptr<FunInst>>(cinst);
      bool secondtry = false;
      known_functions.emplace(f->lv_name, f);
      FunInst tmp = *f;
      auto tmpinss = std::make_shared<MIRblock>(*(tmp.body));
      auto pos = f->body->begin();
      std::vector<std::string> fvlist;
      std::vector<std::string> localvlist;
      auto ccvis = CCVisitor(*this, fvlist, localvlist, pos);
    checkpoint:
      for (auto& a : f->args) {
        localvlist.push_back(a);
      }
      for (auto &it = pos, end = f->body->end(); pos != end; ++it) {
        auto& child = *it;
        ccvis.position=it;
        std::visit(ccvis, child);
      }
      if (!fvlist.empty()) {
        if (secondtry) {
          f->freevariables = fvlist;  // copy;
          // make closure
          std::vector<types::Value> fvtype_inside;
          fvtype_inside.reserve(fvlist.size());
          for (auto& fv : fvlist) {
            fvtype_inside.push_back(typeenv.find(fv));
          }
          auto makecls = std::make_shared<MakeClosureInst>(
              f->lv_name + "_cls", f->lv_name, fvlist,
              types::Tuple(fvtype_inside));
          toplevel->instructions.insert(std::next(it), makecls);
        } else {
          *f = tmp;
          f->body = tmpinss;
          known_functions.erase(f->lv_name);
          pos = f->body->begin();
          fvlist.clear();
          localvlist.clear();
          secondtry = true;
          ccvis.position = f->body->begin();
          goto checkpoint;
        }
      }
    }
  }
  moveFunToTop(this->toplevel);
  // inss.erase(std::unique(inss.begin(), inss.end()), inss.end());
  return this->toplevel;
};  // namespace mimium

bool ClosureConverter::CCVisitor::isFreeVar(const std::string& name) {
  auto islocal =
      std::find(localvlist.begin(), localvlist.end(), name) != localvlist.end();
  bool isext = LLVMBuiltin::isBuiltin(name);
  auto alreadycheked =
      std::find(fvlist.begin(), fvlist.end(), name) != fvlist.end();
  return !(islocal || isext || alreadycheked);
}

void ClosureConverter::CCVisitor::operator()(std::shared_ptr<FunInst> i) {
  bool istoplevel = i->parent->label == "main";
  std::vector<std::string> fvlist;
  std::vector<std::string> localvlist;
  for (auto& a : i->args) {
    localvlist.push_back(a);
  }
  auto pos = i->body->begin();
  auto ccvis = CCVisitor(cc, fvlist, localvlist, pos);
  cc.known_functions.emplace(i->lv_name, i);
  for (auto &it = pos, end = i->body->end(); pos != end; ++it) {
    auto& child = *it;
    std::visit(ccvis, child);
  }
  if (!fvlist.empty()) {
    cc.known_functions.erase(i->lv_name);

    i->freevariables = fvlist;  // copy;
    // make closure
    std::vector<types::Value> fvtype_inside;
    for (auto& fv : fvlist) {
      fvtype_inside.push_back(cc.typeenv.find(fv));
    }
    auto makecls = std::make_shared<MakeClosureInst>(
        i->lv_name + "_cls", i->lv_name, fvlist, types::Tuple(fvtype_inside));
    i->parent->instructions.insert(std::next(position), makecls);
    // cc.toplevel->instructions.insert(cc.toplevel->instructions.begin(), i);
    // auto copy =   std::next(position);
    // i->parent->instructions.erase(position);
    // position = copy;
  }
}

void ClosureConverter::CCVisitor::operator()(std::shared_ptr<NumberInst> i) {
  localvlist.push_back(i->lv_name);
}

void ClosureConverter::CCVisitor::operator()(std::shared_ptr<AllocaInst> i) {
  localvlist.push_back(i->lv_name);
}

void ClosureConverter::CCVisitor::operator()(std::shared_ptr<RefInst> i) {
  if (isFreeVar(i->lv_name)) fvlist.push_back(i->lv_name);
}

void ClosureConverter::CCVisitor::operator()(std::shared_ptr<AssignInst> i) {
  // case of overwrite
  if (isFreeVar(i->lv_name)) fvlist.push_back(i->lv_name);
  if (isFreeVar(i->val)) fvlist.push_back(i->val);
}

void ClosureConverter::CCVisitor::operator()(std::shared_ptr<TimeInst> i) {
  if (isFreeVar(i->lv_name)) fvlist.push_back(i->lv_name);
  if (isFreeVar(i->val)) fvlist.push_back(i->val);
  if (isFreeVar(i->time)) fvlist.push_back(i->time);
}

void ClosureConverter::CCVisitor::operator()(std::shared_ptr<OpInst> i) {
  if (isFreeVar(i->lhs)) fvlist.push_back(i->lhs);
  if (isFreeVar(i->rhs)) fvlist.push_back(i->rhs);
  localvlist.push_back(i->lv_name);
}

void ClosureConverter::CCVisitor::operator()(std::shared_ptr<FcallInst> i) {
  for (auto& a : i->args) {
    if (isFreeVar(a)) fvlist.push_back(a);
  }
  if (cc.isKnownFunction(i->fname)) {
    i->ftype = DIRECT;
  }
  localvlist.push_back(i->lv_name);
}

void ClosureConverter::CCVisitor::operator()(
    std::shared_ptr<MakeClosureInst> i) {
  if (isFreeVar(i->fname)) fvlist.push_back(i->fname);

  localvlist.push_back(i->lv_name);
}

void ClosureConverter::CCVisitor::operator()(std::shared_ptr<ArrayInst> i) {
  // todo
}

void ClosureConverter::CCVisitor::operator()(
    std::shared_ptr<ArrayAccessInst> i) {
  // todo
}

void ClosureConverter::CCVisitor::operator()(std::shared_ptr<IfInst> i) {
  // todo
}

void ClosureConverter::CCVisitor::operator()(std::shared_ptr<ReturnInst> i) {
  if (isFreeVar(i->val)) fvlist.push_back(i->val);
}

}  // namespace mimium