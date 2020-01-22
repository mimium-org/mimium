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
  for (auto it = mir->instructions.begin(), end = mir->instructions.end();
       it != end; ++it) {
    auto& cinst = *it;
    if (std::holds_alternative<FunInst>(cinst)) {
      auto f = std::get<FunInst>(cinst);  // copy
      auto& tinsts = toplevel->instructions;
      moveFunToTop(f.body);
      if (this->toplevel != mir) {
        tinsts.insert(tinsts.begin(), f);  // move on top op toplevel
      }
      f.body->instructions.remove_if([](Instructions v) {
        return std::visit([](auto v) -> bool { return v.isFunction(); }, v);
      });
    }
  }
  toplevel->instructions.sort([](Instructions& i1, Instructions& i2) -> bool {
    auto fn = [](auto& i) { return i.isFunction(); };
    return std::visit(fn, i1) == true && std::visit(fn, i2) == false;
  });
}

std::shared_ptr<MIRblock> ClosureConverter::convert(
    std::shared_ptr<MIRblock> toplevel) {
  // convert top level
  this->toplevel = toplevel;
  auto& inss = toplevel->instructions;
  auto pos = inss.begin();
  std::vector<std::string> fvlist;
  std::vector<std::string> localvlist;
  auto ccvis = CCVisitor(*this, fvlist, localvlist, pos);
  for (auto it = inss.begin(), end = inss.end(); it != end; ++it) {
    auto& cinst = *it;
    std::visit(ccvis, cinst);
  }
  moveFunToTop(this->toplevel);
  // inss.erase(std::unique(inss.begin(), inss.end()), inss.end());
  return this->toplevel;
};  // namespace mimium

bool ClosureConverter::CCVisitor::isFreeVar(const std::string& name) {
  auto islocal =
      std::find(localvlist.begin(), localvlist.end(), name) != localvlist.end();
  bool isext = LLVMBuiltin::isBuiltin(name);
  bool isfun = std::find(funlist.begin(), funlist.end(), name) != funlist.end();

  auto alreadycheked =
      std::find(fvlist.begin(), fvlist.end(), name) != fvlist.end();
  return !(islocal || isext || alreadycheked || isfun);
}

void ClosureConverter::CCVisitor::registerFv(std::string& name) {
  if (isFreeVar(name)) {
    fvlist.push_back(name);
    // auto newname = "fv_" + name;
    // cc.typeenv.emplace(newname, cc.typeenv.find(name));
    // name = newname;
  }
};

void ClosureConverter::CCVisitor::operator()(FunInst& i) {
  funlist.push_back(i.lv_name);
  std::vector<std::string> fvlist;
  std::vector<std::string> localvlist;
  for (auto& a : i.args) {
    localvlist.push_back(a);
  }
  auto pos = i.body->begin();
  auto ccvis = CCVisitor(cc, fvlist, localvlist, pos);
  cc.known_functions.emplace(i.lv_name, 1);
  for (auto &it = pos, end = i.body->end(); pos != end; ++it) {
    auto& child = *it;
    std::visit(ccvis, child);
  }
  if (!fvlist.empty()) {
    cc.known_functions.erase(i.lv_name);

    i.freevariables = fvlist;  // copy;
    // make closure
    std::vector<types::Value> fvtype_inside;
    for (auto& fv : fvlist) {
      fvtype_inside.emplace_back(types::Ref(cc.typeenv.find(fv)));
    }
    auto fvtype = types::Alias(cc.makeCaptureName() ,types::Ref(types::Tuple(fvtype_inside)));
    MakeClosureInst makecls(i.lv_name + "_cls", i.lv_name, fvlist, fvtype);
    i.parent->instructions.insert(std::next(position), std::move(makecls));
    auto& ft = std::get<recursive_wrapper<types::Function>>(i.type).getraw();
    ft.arg_types.emplace_back(fvtype);
  }
}

void ClosureConverter::CCVisitor::operator()(NumberInst& i) {
  localvlist.push_back(i.lv_name);
}

void ClosureConverter::CCVisitor::operator()(AllocaInst& i) {
  localvlist.push_back(i.lv_name);
}

void ClosureConverter::CCVisitor::operator()(RefInst& i) {
  registerFv(i.lv_name);
  localvlist.push_back(i.lv_name);
}

void ClosureConverter::CCVisitor::operator()(AssignInst& i) {
  // case of overwrite
  registerFv(i.lv_name);
  registerFv(i.val);
}

void ClosureConverter::CCVisitor::operator()(TimeInst& i) {
  registerFv(i.val);
  registerFv(i.time);
  localvlist.push_back(i.lv_name);
}

void ClosureConverter::CCVisitor::operator()(OpInst& i) {
  registerFv(i.lhs);
  registerFv(i.rhs);
  localvlist.push_back(i.lv_name);
}

void ClosureConverter::CCVisitor::operator()(FcallInst& i) {
  for (auto& a : i.args) {
    registerFv(a);
  }
  if (cc.isKnownFunction(i.fname)) {
    i.ftype = DIRECT;
  }
  localvlist.push_back(i.lv_name);
}

void ClosureConverter::CCVisitor::operator()(MakeClosureInst& i) {
  // registerFv(i.fname);
  // localvlist.push_back(i.lv_name);
}

void ClosureConverter::CCVisitor::operator()(ArrayInst& i) {
  // todo
}

void ClosureConverter::CCVisitor::operator()(ArrayAccessInst& i) {
  // todo
}

void ClosureConverter::CCVisitor::operator()(IfInst& i) {
  // todo
}

void ClosureConverter::CCVisitor::operator()(ReturnInst& i) {
  registerFv(i.val);
}

}  // namespace mimium