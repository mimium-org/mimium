#include "compiler/closure_convert.hpp"

namespace mimium {
ClosureConverter::ClosureConverter(TypeEnv& _typeenv)
    : typeenv(_typeenv),
      capturecount(0),
      tmp_globalfn("tmp", {}),
      typereplacer(*this) {}

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
  // toplevel->instructions.sort([](Instructions& i1, Instructions& i2) -> bool
  // {
  //   auto fn = [](auto& i) { return i.isFunction(); };
  //   return std::visit(fn, i1) == true && std::visit(fn, i2) == false;
  // });
}

std::shared_ptr<MIRblock> ClosureConverter::convert(
    std::shared_ptr<MIRblock> toplevel) {
  // convert top level
  this->toplevel = toplevel;
  auto& inss = toplevel->instructions;
  auto pos = inss.begin();
  std::vector<std::string> fvlist;
  std::vector<std::string> localvlist;
  std::vector<std::string> funlist;

  auto ccvis = CCVisitor(*this, fvlist, localvlist, funtoclsmap, pos);
  for (auto it = inss.begin(), end = inss.end(); it != end; ++it) {
    auto& cinst = *it;
    ccvis.position = it;
    std::visit(ccvis, cinst);
    std::visit(typereplacer, cinst);
  }
  moveFunToTop(this->toplevel);

  //  typeenv.dump();
  return this->toplevel;
};
void ClosureConverter::CCVisitor::registerFv(std::string& name) {
  auto islocal =
      std::find(localvlist.begin(), localvlist.end(), name) != localvlist.end();
  bool isext = LLVMBuiltin::isBuiltin(name);
  // bool isknownfun =
  // cc.isKnownFunction(name);//std::holds_alternative<Rec_Wrap<types::Function>>(
  // cc.typeenv.find(name));
  auto alreadycheked =
      std::find(fvlist.begin(), fvlist.end(), name) != fvlist.end();
  bool isfreevar = !(islocal || isext || alreadycheked);
  if (isfreevar) {
    fvlist.push_back(name);
  }
  // if (isfun && cc.known_functions.count(name) >= 0) {
  //   name = name + "_cls";
  // }
};

void ClosureConverter::CCVisitor::operator()(FunInst& i) {
  this->localvlist.push_back(i.lv_name);
  std::vector<std::string> fvlist;
  std::vector<std::string> localvlist;
  for (auto& a : i.args) {
    localvlist.push_back(a);
  }
  auto pos = i.body->begin();
  auto ccvis = CCVisitor(cc, fvlist, localvlist, funtoclsmap, pos);
  cc.known_functions.emplace(i.lv_name, 1);
  bool checked = false;
checkpoint:
  for (auto &it = pos, end = i.body->end(); pos != end; ++it) {
    auto& child = *it;
    ccvis.position = it;
    std::visit(ccvis, child);
    std::visit(cc.typereplacer, child);
  }
  if (!fvlist.empty()) {
    cc.known_functions.erase(i.lv_name);
    if (!checked) {
      checked = true;
      goto checkpoint;  // NOLINT
    }
    // make closure
    i.freevariables = fvlist;  // copy;
    std::vector<types::Value> fvtype_inside;
    fvtype_inside.reserve(fvlist.size());
    for (auto& fv : fvlist) {
      fvtype_inside.emplace_back(cc.typeenv.find(fv));
    }

    auto clsname = i.lv_name + "_cls";
    auto fvtype =
        types::Alias(cc.makeCaptureName(), types::Tuple(fvtype_inside));
    auto ftype = rv::get<types::Function>(i.type);
    ftype.arg_types.emplace_back(types::Ref(fvtype));
    auto clstype = types::Alias(cc.makeClosureTypeName(),
                                types::Closure(types::Ref(ftype), fvtype));

    MakeClosureInst makecls(clsname, i.lv_name, fvlist, clstype);

    i.parent->instructions.insert(std::next(position), makecls);
    cc.typeenv.emplace(clsname, clstype);
    // replace original function type
    cc.typeenv.emplace(i.lv_name, clstype);
    // auto& ft = std::get<Rec_Wrap<types::Function>>(i.type).getraw();
    // ft.arg_types.emplace_back(fvtype);
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
  if(cc.isKnownFunction(i.fname)){
    i.ftype = DIRECT;
  }else{
    registerFv(i.fname);
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