#include "compiler/closure_convert.hpp"

namespace mimium {
ClosureConverter::ClosureConverter(TypeEnv& _typeenv)
    : typeenv(_typeenv), capturecount(0),tmp_globalfn("tmp",{}){}

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
  std::vector<std::string> fvlist_toplevel;
  for (auto it = toplevel->begin(), end = toplevel->end(); it != end; ++it) {
    auto& child = *it;
    std::visit(CCVisitor(*this, fvlist_toplevel, it), child);
  }
  this->toplevel = toplevel;
  moveFunToTop(this->toplevel);
  return this->toplevel;
};

bool ClosureConverter::CCVisitor::isFreeVar(const std::string& name) {
  auto search_res = std::find(localvlist.begin(), localvlist.end(), name);
  return search_res == localvlist.end();
}

void ClosureConverter::CCVisitor::operator()(std::shared_ptr<FunInst> i) {
  if (i->parent->label == "main") {
    cc.tmp_globalfn = *i;  // copy
  }
  cc.known_functions.emplace(i->lv_name, i);
  std::vector<std::string> fvlist;
  auto pos = i->body->begin();
  auto ccvis = CCVisitor(cc, fvlist, pos);
  for (auto &it = pos, end = i->body->end(); pos != end; ++it) {
    auto& child = *it;
    std::visit(ccvis, child);
    if (!fvlist.empty()) break;
  }
  if (!fvlist.empty()) {
    // reset toplevel state and retry
        cc.known_functions.erase(i->lv_name);

    if (i->parent->label == "main") {
      *i = cc.tmp_globalfn;  // copy
    }else{
      return;//early return
    }
    std::vector<std::string> fvlist2;
    pos = i->body->begin();
    auto ccvis2 = CCVisitor(cc, fvlist2, pos);
    for (auto &it = pos, end = i->body->end(); pos != end; ++it) {
      auto& child = *it;
      std::visit(ccvis2, child);
    }
    i->freevariables = fvlist2;  // copy;
    // make closure
    std::vector<types::Value> fvtype_inside;
    fvtype_inside.reserve(fvlist2.size());
    for (auto& fv : fvlist2) {
      fvtype_inside.push_back(cc.typeenv.find(fv));
    }
    auto makecls = std::make_shared<MakeClosureInst>(
        i->lv_name + "_cls", i->lv_name, fvlist2, types::Tuple(fvtype_inside));
    i->parent->instructions.insert(++position, makecls);
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