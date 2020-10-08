/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/closure_convert.hpp"

namespace mimium {
ClosureConverter::ClosureConverter(TypeEnv& typeenv)
    : typeenv(typeenv),
      capturecount(0),
      closurecount(0)
// ,typereplacer(*this)
{
  known_functions.emplace("mimium_getnow", 1);
}

void ClosureConverter::reset() { capturecount = 0; }
ClosureConverter::~ClosureConverter() = default;

bool ClosureConverter::isKnownFunction(const std::string& name) {
  return known_functions.find(name) != known_functions.end();
}

void ClosureConverter::moveFunToTop(mir::blockptr mir) {
  auto& tinsts = toplevel->instructions;
  for (auto it = mir->instructions.begin(), end = mir->instructions.end(); it != end; ++it) {
    auto& cinst = *it;
    if (std::holds_alternative<mir::FunInst>(cinst)) {
      auto f = std::get<mir::FunInst>(cinst);  // copy
      moveFunToTop(f.body);                    // recursive call
      if (this->toplevel != mir) {
        tinsts.insert(tinsts.begin(), f);  // move on toplevel
      }
      f.body->instructions.remove_if(
          [](mir::Instructions v) { return std::holds_alternative<mir::FunInst>(v); });
    }
  }
}

mir::blockptr ClosureConverter::convert(mir::blockptr toplevel) {
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
    // std::visit(typereplacer, cinst);
  }
  moveFunToTop(this->toplevel);
  if (!(clstypeenv.count("dsp") > 0)) {
    types::Value dummycapture = types::Alias{makeCaptureName(), types::Tuple{{}}};
    auto ctypename = makeClosureTypeName();
    auto dummytype = types::Alias{
        ctypename, types::Closure{types::Ref{types::Function{
                                      types::Float{}, {types::Float{}, types::Ref{dummycapture}}}},
                                  dummycapture}};
    clstypeenv.emplace("dsp", dummycapture);
    // typeenv.emplace("dsp_cls", std::move(dummytype));
  }
  return this->toplevel;
};

void ClosureConverter::dump() {
  std::cerr << "----------fvinfo-----------\n";
  for (auto&& [key, arr] : fvinfo) {
    std::cerr << key << " : {";
    std::for_each(arr.begin(), arr.end(), [](auto& v) { std::cerr << v << " "; });
    std::cerr << "}\n";
  }
  std::cerr << "----------typeinfo-----------\n";
  for (auto&& [key, val] : clstypeenv) {
    std::cerr << key << " : " << types::toString(val, true) << "\n";
  }
}

void ClosureConverter::CCVisitor::registerFv(std::string& name) {
  auto isself = name == "self";
  auto islocal = has(localvlist, name);
  bool isext = LLVMBuiltin::isBuiltin(name);
  auto alreadycheked = has(fvlist, name);
  bool isfreevar = !(islocal || isext || alreadycheked || isself);
  if (isfreevar) { fvlist.push_back(name); }
};

void ClosureConverter::CCVisitor::visitinsts(mir::FunInst& i, CCVisitor& ccvis,
                                             std::list<mir::Instructions>::iterator pos) {
  for (auto &it = pos, end = i.body->instructions.end(); pos != end; ++it) {
    auto& child = *it;
    ccvis.position = it;
    std::visit(ccvis, child);
    // std::visit(cc.typereplacer, child);
  }
}

void ClosureConverter::CCVisitor::operator()(mir::FunInst& i) {
  this->localvlist.push_back(i.lv_name);
  std::vector<std::string> fvlist;
  std::vector<std::string> localvlist = {i.lv_name};
  for (auto& a : i.args) { localvlist.emplace_back(a); }
  auto pos = std::begin(i.body->instructions);
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
    std::vector<types::Value> fvtype_inside;
    fvtype_inside.reserve(fvlist.size());
    auto it = std::begin(fvlist);
    for (auto& fv : fvlist) {
      bool isrecurse = fv == i.lv_name;
      if (!isrecurse) {
        auto ft = cc.typeenv.find(fv);
        if (rv::holds_alternative<types::Function>(ft)) { ft = cc.typeenv.find(fv + "_cls"); }
        fvtype_inside.emplace_back(types::Ref{ft});
      } else {
        fvlist.erase(it);
      }
      ++it;
    }
    cc.fvinfo.emplace(i.lv_name, fvlist);

    i.freevariables = fvlist;  // copy;

    auto clsname = i.lv_name + "_cls";
    // do not use auto here, move happens...
    types::Alias fvtype{cc.makeCaptureName(), types::Tuple{fvtype_inside}};
    types::Function ftype = rv::get<types::Function>(cc.typeenv.find(i.lv_name));
    // types::Alias clstype{cc.makeClosureTypeName(),
    //                      types::Closure{types::Ref{types::Function{ftype}},
    //                      types::Alias{fvtype}}};

    auto makecls = createClosureInst(ftype, fvtype, i.lv_name);

    i.parent->instructions.insert(std::next(position), makecls);

    // replace original function type
    // cc.typeenv.emplace(i.lv_name, clstype);
    // auto& ft = std::get<Box<types::Function>>(i.type).getraw();
    // ft.arg_types.emplace_back(fvtype);
  }
}
mir::MakeClosureInst ClosureConverter::CCVisitor::createClosureInst(types::Function ftype,
                                                                    types::Alias fvtype,
                                                                    std::string& lv_name) {
  auto clsname = lv_name + "_cls";
  ftype.arg_types.emplace_back(types::Ref{fvtype});
  types::Alias clstype{cc.makeClosureTypeName(), types::Closure{types::Ref{ftype}, fvtype}};
  mir::MakeClosureInst makecls{{clsname}, lv_name, fvlist, fvtype};
  cc.typeenv.emplace(clsname, fvtype);
  cc.clstypeenv.emplace(lv_name, fvtype);
  return makecls;
}

void ClosureConverter::CCVisitor::operator()(mir::RefInst& i) {
  registerFv(i.lv_name);
  localvlist.push_back(i.lv_name);
}

void ClosureConverter::CCVisitor::operator()(mir::AssignInst& i) {
  // case of overwrite
  registerFv(i.lv_name);
  registerFv(i.val);
}
void ClosureConverter::CCVisitor::operator()(mir::OpInst& i) {
  if (!i.lhs.empty()) { registerFv(i.lhs); }
  registerFv(i.rhs);
  localvlist.push_back(i.lv_name);
}

void ClosureConverter::CCVisitor::operator()(mir::FcallInst& i) {
  for (auto& a : i.args) { registerFv(a); }
  if (i.time) { registerFv(i.time.value()); }
  if (cc.isKnownFunction(i.fname)) {
    i.ftype = DIRECT;
  } else {
    if (i.ftype != EXTERNAL) {
      i.ftype = CLOSURE;
      // auto clsname = i.fname+"_cls";
      registerFv(i.fname);
    }
  }
  localvlist.push_back(i.lv_name);
}

void ClosureConverter::CCVisitor::operator()(mir::MakeClosureInst& i) {
  // registerFv(i.fname);
  // localvlist.push_back(i.lv_name);
}

void ClosureConverter::CCVisitor::operator()(mir::ArrayInst& i) {
  for (auto& e : i.args) { registerFv(e); }
  localvlist.push_back(i.lv_name);
}

void ClosureConverter::CCVisitor::operator()(mir::ArrayAccessInst& i) {
  registerFv(i.name);
  registerFv(i.index);
  localvlist.push_back(i.lv_name);
}
void ClosureConverter::CCVisitor::operator()(mir::FieldInst& i) {
  registerFv(i.name);
  if (auto* iname = std::get_if<std::string>(&i.index)) { registerFv(*iname); }
  localvlist.push_back(i.lv_name);
}

void ClosureConverter::CCVisitor::operator()(mir::IfInst& i) {
  registerFv(i.cond);
  for (auto& ti : i.thenblock->instructions) { std::visit(*this, ti); }
  if (i.elseblock.has_value()) {
    for (auto& ei : i.elseblock.value()->instructions) { std::visit(*this, ei); }
  }
  localvlist.push_back(i.lv_name);
}

void ClosureConverter::CCVisitor::operator()(mir::ReturnInst& i) { registerFv(i.val); }

}  // namespace mimium