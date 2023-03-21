/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/closure_convert.hpp"
#include <iostream>
#include <vector>
namespace mimium {
ClosureConverter::ClosureConverter() : capturecount(0), closurecount(0) {}

void ClosureConverter::reset() { capturecount = 0; }
ClosureConverter::~ClosureConverter() = default;

bool ClosureConverter::isKnownFunction(mir::valueptr fn) {
  return known_functions.find(fn) != known_functions.end();
}

void ClosureConverter::moveFunToTop(mir::blockptr mir) {
  auto& tinsts = toplevel->instructions;
  std::list<decltype(mir->instructions)::iterator> inst_tobe_removed;
  for (auto &&it = mir->instructions.begin(), end = mir->instructions.end(); it != end; ++it) {
    mir::valueptr cinst = *it;
    if (auto* f = std::get_if<minst::Function>(&std::get<mir::Instructions>(*cinst))) {
      moveFunToTop(f->body);  // recursive call
      if (this->toplevel != mir) {
        tinsts.insert(tinsts.begin(), cinst);  // move on toplevel
        inst_tobe_removed.emplace_back(it);
      }
    }
  }
  for (auto& it : inst_tobe_removed) { mir->instructions.erase(it); }
}

mir::blockptr ClosureConverter::convert(mir::blockptr toplevel) {
  // convert top level
  this->toplevel = toplevel;
  auto& inss = toplevel->instructions;
  auto pos = inss.begin();
  std::vector<std::string> fvset;
  std::vector<std::string> localvlist;
  std::vector<std::string> funlist;

  auto ccvis = CCVisitor(*this, pos);

  ccvis.block_ctx = toplevel;
  for (auto it = inss.begin(), end = inss.end(); it != end; ++it) {
    auto& cinst = *it;
    ccvis.position = it;
    ccvis.instance_holder = cinst;
    ccvis.visit(*cinst);
  }

  moveFunToTop(this->toplevel);
  if (!(clstypeenv.count("dsp") > 0)) {
    auto dummycapture =
        LType::Value{LType::Alias{makeCaptureName(), LType::Value{LType::Tuple{{}}}}};
    clstypeenv.emplace("dsp", dummycapture);
  }
  return this->toplevel;
}  // namespace mimium

void ClosureConverter::CCVisitor::dump() {
  std::cerr << "----------fvset-----------\n";
  for (const auto& v : fvset) { std::cerr << mir::getName(*v) << "\n"; }
  std::flush(std::cerr);
}

void ClosureConverter::CCVisitor::checkFreeVar(const mir::valueptr val) {
  if (auto* iptr = std::get_if<mir::Instructions>(val.get())) {
    bool isfun = LType::canonicalCheck<LType::Function>(mir::getType(*iptr));
    bool has_different_root = mir::getParent(*iptr) != this->block_ctx;
    if (!isfun && has_different_root) { fvset.emplace(val); }
  }
}
void ClosureConverter::CCVisitor::checkFreeVarArg(const mir::valueptr val) {
  if (auto* iptr = std::get_if<std::shared_ptr<mir::Argument>>(val.get())) {
    mir::valueptr ctx = this->block_ctx->parent.value();
    if ((*iptr)->parentfn != ctx) { fvset.emplace(val); }
  }
}
void ClosureConverter::CCVisitor::checkFreeVar(const mir::blockptr block) {
  // mostly for visiting for if statement block
  for (auto& inst : block->instructions) {
    this->instance_holder = inst;
    std::visit(*this, std::get<mir::Instructions>(*inst));
  }
}

void ClosureConverter::CCVisitor::checkVariable(mir::valueptr& val, bool ismemoryvalue) {
  tryReplaceFntoCls(val);
  if (ismemoryvalue) {
    checkFreeVar(val);
  } else {
    checkFreeVarArg(val);
  }
}
void ClosureConverter::CCVisitor::tryReplaceFntoCls(mir::valueptr& val) {
  auto iter = cc.fn_to_cls.find(val);
  if (iter != cc.fn_to_cls.end()) { val = cc.fn_to_cls[val]; }
}

void ClosureConverter::CCVisitor::visitinsts(minst::Function& i, CCVisitor& ccvis) {
  for (auto it = i.body->instructions.begin(), end = i.body->instructions.end(); it != end; ++it) {
    ccvis.position = it;
    mir::valueptr instance = *it;
    ccvis.instance_holder = instance;
    if (auto* i = std::get_if<mir::Instructions>(it->get())) { std::visit(ccvis, *i); }
    // std::visit(cc.typereplacer, child);
  }
}

void ClosureConverter::CCVisitor::operator()(minst::Function& i) {
  auto pos = std::begin(i.body->instructions);
  auto ccvis = CCVisitor(cc, pos);
  auto know_function_tmp = cc.known_functions;  // copy
  auto fn_ptr = getValPtr(&i);
  auto stored_fn_iter = cc.known_functions.insert(cc.known_functions.end(), fn_ptr);
  ccvis.block_ctx = i.body;
  // first, try assuming the function is not a closure.
  visitinsts(i, ccvis);
  // when the function was really not a closure, end.
  if (ccvis.fvset.empty()) { return; }
  // if the function refers free variable, try conversion again-
  // by assuming the function is closure.

  cc.known_functions.erase(stored_fn_iter);
  ccvis.fvset.clear();
  visitinsts(i, ccvis);

  // make closure
  List<Box<LType::Value>> fvtype_inside;
  auto it = std::begin(ccvis.fvset);
  for (const auto& fv : ccvis.fvset) {
    bool isrecurse = mir::toString(*fv) == i.name;
    if (!isrecurse) {
      auto ft = mir::getType(*fv);
      fvtype_inside.emplace_back(ft);
    } else {
      ccvis.fvset.erase(it);
    }
    ++it;
  }
  auto fvsetvec = fmap<std::set, List>(ccvis.fvset, [](auto i) { return i; });
  i.freevariables = fvsetvec;  // copy;
  // do not use auto here, move happens...
  LType::Alias fvtype{cc.makeCaptureName(), LType::Value{LType::Tuple{fvtype_inside}}};

  auto& ftype = LType::getCanonicalType<LType::Function>(i.type);
  ftype.v.first.emplace_back(LType::Value{LType::Pointer{LType::Value{fvtype}}});
  auto makecls = std::make_shared<mir::Value>(
      createClosureInst(fn_ptr, fvsetvec, LType::Value{fvtype}, i.name));
  cc.fn_to_cls.emplace(fn_ptr, makecls);
  i.parent->instructions.insert(std::next(position), makecls);
}
minst::MakeClosure ClosureConverter::CCVisitor::createClosureInst(mir::valueptr fnptr,
                                                                  List<mir::valueptr> const& fvs,
                                                                  LType::Value fvtype,
                                                                  std::string& lv_name) {
  auto clsname = lv_name + "_cls";
  auto ftype = mir::getType(*fnptr);
  LType::getCanonicalType<LType::Function>(ftype).v.first.emplace_back(LType::Value{LType::Pointer{fvtype}});
  LType::Alias clstype{cc.makeClosureTypeName(),
                       makeClosureType(LType::Pointer{mir::getType(*fnptr)}, fvtype)};
  minst::MakeClosure makecls{{clsname, LType::Value{clstype}}, fnptr, fvs};
  return makecls;
}

void ClosureConverter::CCVisitor::operator()(minst::Ref& i) { checkVariable(i.target, true); }

void ClosureConverter::CCVisitor::operator()(minst::Load& i) { checkVariable(i.target, true); }
void ClosureConverter::CCVisitor::operator()(minst::Store& i) { checkVariable(i.target, true); }
// void ClosureConverter::CCVisitor::operator()(minst::Op& i) {
//   if (i.lhs.has_value()) { checkVariable(i.lhs.value()); }
//   checkVariable(i.rhs);
// }

void ClosureConverter::CCVisitor::operator()(minst::Fcall& i) {
  checkVariable(i.fname, true);
  checkVariable(i.fname);
  if (i.time.has_value()) { checkVariable(i.time.value()); }
  for (auto& a : i.args) { checkVariable(a); }
  // currently higher order function is limited to direct call - no closure or memobj
  const bool is_hof = std::holds_alternative<std::shared_ptr<mir::Argument>>(*i.fname);
  if (cc.isKnownFunction(i.fname) || is_hof) {
    i.ftype = DIRECT;
  } else {
    if (i.ftype != EXTERNAL) { i.ftype = CLOSURE; }
  }
}

void ClosureConverter::CCVisitor::operator()(minst::MakeClosure& i) {
  // registerFv(i.fname);
  // localvlist.push_back(i.lv_name);
}

void ClosureConverter::CCVisitor::operator()(minst::Array& i) {
  for (auto& a : i.args) { checkVariable(a); }
}
void ClosureConverter::CCVisitor::operator()(minst::ArrayAccess& i) {
  checkFreeVar(i.target);
  checkVariable(i.target);
  checkVariable(i.index);
}

void ClosureConverter::CCVisitor::operator()(minst::Field& i) {
  checkFreeVar(i.target);
  checkVariable(i.target);
  checkVariable(i.index);
}

void ClosureConverter::CCVisitor::operator()(minst::If& i) {
  checkVariable(i.cond);
  checkFreeVar(i.thenblock);
  if (i.elseblock.has_value()) { checkFreeVar(i.elseblock.value()); }
}

void ClosureConverter::CCVisitor::operator()(minst::Return& i) { checkVariable(i.val); }
}  // namespace mimium