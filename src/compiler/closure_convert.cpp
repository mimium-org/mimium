/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
 
#include "compiler/closure_convert.hpp"

namespace mimium {
ClosureConverter::ClosureConverter(TypeEnv& typeenv)
    : typeenv(typeenv),
      capturecount(0),
      closurecount(0),
      tmp_globalfn("tmp", {}),
      typereplacer(*this) {}

void ClosureConverter::reset() { capturecount = 0; }
ClosureConverter::~ClosureConverter() = default;

bool ClosureConverter::isKnownFunction(const std::string& name) {
  return known_functions.find(name) != known_functions.end();
}

void ClosureConverter::moveFunToTop(std::shared_ptr<MIRblock> mir) {
  auto& tinsts = toplevel->instructions;
  for (auto it = mir->instructions.begin(), end = mir->instructions.end();
       it != end; ++it) {
    auto& cinst = *it;
    if (std::holds_alternative<FunInst>(cinst)) {
      auto f = std::get<FunInst>(cinst);  // copy
      moveFunToTop(f.body);
      if (this->toplevel != mir) {
        tinsts.insert(tinsts.begin(), f);  // move on top op toplevel
      }
      f.body->instructions.remove_if([](Instructions v) {
        return std::visit([](auto v) -> bool { return v.isFunction(); }, v);
      });
    }
  }
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

  auto ccvis = CCVisitor(*this, fvlist, localvlist, pos);
  for (auto it = inss.begin(), end = inss.end(); it != end; ++it) {
    auto& cinst = *it;
    ccvis.position = it;
    std::visit(ccvis, cinst);
    std::visit(typereplacer, cinst);
  }
  moveFunToTop(this->toplevel);
  if(!typeenv.exist("dsp_cls")){
    auto dummycapture= types::Alias(makeCaptureName(),types::Tuple({}));
    auto dummytype = types::Alias(makeClosureTypeName(),types::Closure(types::Ref(types::Function(types::Float(),{types::Float(),types::Ref(dummycapture)})),dummycapture));
    typeenv.emplace("dsp_cls", std::move(dummytype));
  }
  return this->toplevel;
};
void ClosureConverter::CCVisitor::registerFv(std::string& name) {
  auto isself = name == "self";
  auto islocal = has(localvlist, name);
  bool isext = LLVMBuiltin::isBuiltin(name);
  auto alreadycheked = has(fvlist, name);
  bool isfreevar = !(islocal || isext || alreadycheked || isself);
  if (isfreevar) {
    fvlist.push_back(name);
  }
};

void ClosureConverter::CCVisitor::visitinsts(
    FunInst& i, CCVisitor& ccvis, std::list<Instructions>::iterator pos) {
  for (auto &it = pos, end = i.body->end(); pos != end; ++it) {
    auto& child = *it;
    ccvis.position = it;
    std::visit(ccvis, child);
    std::visit(cc.typereplacer, child);
  }
}

void ClosureConverter::CCVisitor::operator()(FunInst& i) {
  this->localvlist.push_back(i.lv_name);
  std::vector<std::string> fvlist;
  std::vector<std::string> localvlist= {i.lv_name};
  for (auto& a : i.args) {
    localvlist.emplace_back(a);
  }
  auto pos = i.body->begin();
  auto ccvis = CCVisitor(cc, fvlist, localvlist, pos);
  cc.known_functions.emplace(i.lv_name, 1);
  bool checked = false;
checkpoint:
  visitinsts(i, ccvis, pos);
  if (!fvlist.empty()) {
    cc.known_functions.erase(i.lv_name);
    if (!checked) {
      checked = true;
      goto checkpoint;  // NOLINT
    }
    // if (i.isrecursive) {//to replace recursive call to appcls
    //   visitinsts(i, ccvis,pos);
    // }
    // make closure
    i.freevariables = fvlist;  // copy;
    std::vector<types::Value> fvtype_inside;
    fvtype_inside.reserve(fvlist.size());
    for (auto& fv : fvlist) {
      auto ft = cc.typeenv.find(fv);
      fvtype_inside.emplace_back(types::Ref(ft));
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
  if (cc.isKnownFunction(i.fname)) {
    i.ftype = DIRECT;
  } else {
    if (i.ftype != EXTERNAL) {
      i.ftype = CLOSURE;
    }
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