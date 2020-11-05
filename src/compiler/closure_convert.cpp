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
{}

void ClosureConverter::reset() { capturecount = 0; }
ClosureConverter::~ClosureConverter() = default;

bool ClosureConverter::isKnownFunction(mir::valueptr fn) {
  return std::binary_search(known_functions.begin(), known_functions.end(), fn);
}

void ClosureConverter::moveFunToTop(mir::blockptr mir) {
  auto& tinsts = toplevel->instructions;
  for (auto it = mir->instructions.begin(), end = mir->instructions.end(); it != end; ++it) {
    mir::valueptr cinst = *it;
    if (auto* f = std::get_if<minst::Function>(&std::get<mir::Instructions>(*cinst))) {
      moveFunToTop(f->body);  // recursive call
      if (this->toplevel != mir) {
        tinsts.insert(tinsts.begin(), cinst);  // move on toplevel
        tinsts.erase(it);
      }
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

  auto ccvis = CCVisitor(*this, pos);

  ccvis.block_ctx = toplevel;
  for (auto it = inss.begin(), end = inss.end(); it != end; ++it) {
    auto& cinst = *it;
    ccvis.position = it;
    if (auto* i = std::get_if<mir::Instructions>(cinst.get())) {
      if (std::holds_alternative<mir::instruction::Function>(*i)) { ccvis.visit(*cinst); }
    }
  }
  // std::visit(typereplacer, cinst);

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
};  // namespace mimium

void ClosureConverter::CCVisitor::dump() {
  std::cerr << "----------fvlist-----------\n";
  for (auto& v : fvlist) { std::cerr << mir::getName(*v) << "\n"; }
  std::flush(std::cerr);
}

void ClosureConverter::CCVisitor::checkFreeVar(mir::valueptr val) {
  if (auto* iptr = std::get_if<mir::Instructions>(val.get())) {
    bool isfun = std::holds_alternative<types::rFunction>(mir::getType(*iptr));
    bool has_different_root = mir::getParent(*iptr) != this->block_ctx;
    if (!isfun && has_different_root) { fvlist.emplace_back(val); }
  }
=}

void ClosureConverter::CCVisitor::checkFreeVar(mir::blockptr block) {
  for (auto& inst : block->instructions) { checkFreeVar(inst); }
}

// void ClosureConverter::CCVisitor::registerFv(std::string& name) {
//   auto isself = name == "self";
//   auto islocal = has(localvlist, name);
//   bool isext = LLVMBuiltin::isBuiltin(name);
//   auto alreadycheked = has(fvlist, name);
//   bool isfreevar = !(islocal || isext || alreadycheked || isself);
//   if (isfreevar) { fvlist.push_back(name); }
// };

void ClosureConverter::CCVisitor::visitinsts(minst::Function& i, CCVisitor& ccvis) {
  for (auto it = i.body->instructions.begin(), end = i.body->instructions.end(); it != end; ++it) {
    ccvis.position = it;
    if (auto* i = std::get_if<mir::Instructions>(it->get())) { std::visit(ccvis, *i); }
    // std::visit(cc.typereplacer, child);
  }
}

void ClosureConverter::CCVisitor::operator()(minst::Function& i) {
  auto pos = std::begin(i.body->instructions);
  auto ccvis = CCVisitor(cc, pos);
  auto ptr = *pos;
  auto know_function_tmp = cc.known_functions;  // copy
  auto stored_fn_iter = cc.known_functions.insert(cc.known_functions.end(), ptr);
  this->block_ctx = i.body;
  bool checked = false;
checkpoint:
  visitinsts(i, ccvis);
  if (!fvlist.empty()) {
    cc.known_functions = know_function_tmp;
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
      bool isrecurse = mir::toString(*fv) == i.name;
      if (!isrecurse) {
        auto ft = mir::getType(*fv);
        // if (rv::holds_alternative<types::Function>(ft)) { ft = cc.typeenv.find(fv + "_cls"); }
        fvtype_inside.emplace_back(types::Ref{ft});
      } else {
        fvlist.erase(it);
      }
      ++it;
    }
    std::vector<mir::valueptr> fvlistvec;
    std::transform(fvlist.begin(), fvlist.end(), std::back_inserter(fvlistvec),
                   [](auto& i) { return i; });
    i.freevariables = fvlistvec;  // copy;

    // do not use auto here, move happens...
    types::Alias fvtype{cc.makeCaptureName(), types::Tuple{fvtype_inside}};
    types::Function ftype = rv::get<types::Function>(i.type);
    // types::Alias clstype{cc.makeClosureTypeName(),
    //                      types::Closure{types::Ref{types::Function{ftype}},
    //                      types::Alias{fvtype}}};

    auto makecls = createClosureInst(ftype, fvtype, i.name);

    i.parent->instructions.insert(std::next(position),
                                  std::make_shared<mir::Value>(std::move(makecls)));

    // replace original function type
    // cc.typeenv.emplace(i.lv_name, clstype);
    // auto& ft = std::get<Box<types::Function>>(i.type).getraw();
    // ft.arg_types.emplace_back(fvtype);
  }
}
minst::MakeClosure ClosureConverter::CCVisitor::createClosureInst(types::Function ftype,
                                                                  types::Alias fvtype,
                                                                  std::string& lv_name) {
  auto clsname = lv_name + "_cls";
  ftype.arg_types.emplace_back(types::Ref{fvtype});
  types::Alias clstype{cc.makeClosureTypeName(), types::Closure{types::Ref{ftype}, fvtype}};
  std::vector<mir::valueptr> fvs;
  std::transform(fvlist.begin(), fvlist.end(), std::back_inserter(fvs),
                 [](mir::valueptr p) { return p; });
  minst::MakeClosure makecls{{clsname}, block_ctx->parent.value(), fvs, fvtype};
  cc.typeenv.emplace(clsname, fvtype);
  cc.clstypeenv.emplace(lv_name, fvtype);
  return makecls;
}

void ClosureConverter::CCVisitor::operator()(minst::Ref& i) { checkFreeVar(i.target); }

void ClosureConverter::CCVisitor::operator()(minst::Load& i) { checkFreeVar(i.target); }
void ClosureConverter::CCVisitor::operator()(minst::Store& i) {
  checkFreeVar(i.target);
  checkFreeVar(i.value);
}
void ClosureConverter::CCVisitor::operator()(minst::Op& i) {
  if (i.lhs.has_value()) { checkFreeVar(i.lhs.value()); }
  checkFreeVar(i.rhs);
}

void ClosureConverter::CCVisitor::operator()(minst::Fcall& i) {
  for (auto& a : i.args) { checkFreeVar(a); }
  if (i.time) { checkFreeVar(i.time.value()); }
  if (cc.isKnownFunction(i.fname)) {
    i.ftype = DIRECT;
  } else {
    if (i.ftype != EXTERNAL) {
      i.ftype = CLOSURE;
      // auto clsname = i.fname+"_cls";
      checkFreeVar(i.fname);
    }
  }
}

void ClosureConverter::CCVisitor::operator()(minst::MakeClosure& i) {
  // registerFv(i.fname);
  // localvlist.push_back(i.lv_name);
}

void ClosureConverter::CCVisitor::operator()(minst::Array& i) {
  for (auto& e : i.args) { checkFreeVar(e); }
}

void ClosureConverter::CCVisitor::operator()(minst::Field& i) {
  checkFreeVar(i.target);
  checkFreeVar(i.index);
}

void ClosureConverter::CCVisitor::operator()(minst::If& i) {
  checkFreeVar(i.cond);
  checkFreeVar(i.thenblock);
  if (i.elseblock.has_value()) { checkFreeVar(i.elseblock.value()); }
}

void ClosureConverter::CCVisitor::operator()(minst::Return& i) { checkFreeVar(i.val); }

}  // namespace mimium