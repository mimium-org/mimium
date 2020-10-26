/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "collect_memoryobjs.hpp"
namespace mimium {

MemoryObjsCollector::MemoryObjsCollector(TypeEnv& typeenv) : typeenv(typeenv) {}

funmap MemoryObjsCollector::collectToplevelFuns(mir::blockptr toplevel) {
  funmap res;
  for (auto&& inst : toplevel->instructions) {
    if (auto* iptr = std::get_if<mir::Instructions>(inst.get())) {
      if (auto* funptr = std::get_if<minst::Function>(iptr)) { res.emplace(funptr->name, funptr); }
    }
  }
  return res;
}

std::shared_ptr<FunObjTree> MemoryObjsCollector::traverseFunTree(minst::Function const& f) {
  std::string name = f.name;
  CollectMemVisitor visitor(*this, f);
  for (auto&& i : f.body->instructions) { visitor.visit(i); }
  if (visitor.res_hasself) {
    auto fntype = f.type;
    auto rettype = rv::get<types::Function>(fntype).ret_type;
    visitor.objtype.arg_types.emplace_back(rettype);
  }
  auto objptr = std::make_shared<FunObjTree>();
  *objptr = FunObjTree{name, visitor.res_hasself, visitor.obj, visitor.objtype};
  result_map.emplace(name, objptr);
  return objptr;
}

funobjmap& MemoryObjsCollector::process(mir::blockptr toplevel) {
  result_map.emplace(
      "delay", std::make_shared<FunObjTree>(FunObjTree{"delay", false, {}, types::delaystruct}));
  this->toplevel_funmap = collectToplevelFuns(toplevel);
  if (toplevel_funmap.count("dsp") > 0) {
    auto* dspfun = toplevel_funmap.at("dsp");
    auto res = traverseFunTree(*dspfun);
    auto& insts = toplevel->instructions;
    auto dspmemtype = res->objtype;
    auto alloca = minst::Allocate{{"dsp.mem", dspmemtype}, dspmemtype};
    insts.insert(std::begin(insts), std::make_shared<mir::Value>(std::move(alloca)));
    typeenv.emplace("dsp.mem", dspmemtype);
  } else {
    // create dummy dspmemobj
    result_map.emplace("dsp",
                       std::make_shared<FunObjTree>(FunObjTree{"dsp", false, {}, types::Tuple{}}));
  }

  return result_map;
}

std::string MemoryObjsCollector::indentHelper(int indent) {
  std::string indent_s;
  for (int i = 0; i < indent; i++) { indent_s += " "; }
  return indent_s;
}

void MemoryObjsCollector::dumpFunObjTree(FunObjTree const& tree, int indent) {
  std::string fn_s = indentHelper(indent) + tree.fname + "->";
  std::cerr << fn_s;
  if (tree.hasself) { std::cerr << "self\n" << indentHelper(fn_s.size()); }
  for (auto&& o : tree.memobjs) {
    dumpFunObjTree(*o, fn_s.size());
    std::cerr << "\n";
  }
}

void MemoryObjsCollector::dump() {
  std::cerr << "-------------Memory Object Functions: -----\n";
  dumpFunObjTree(res);

  // std::cerr << "-------------type alias Map: -----\n";
  // for (auto&& [key, val] : type_alias_map) {
  //   std::cerr << key << " : " << types::toString(val, true) << "\n";
  // }
  std::cerr << "------------\n";
}

// visitor

void MemoryObjsCollector::CollectMemVisitor::operator()(minst::Ref& i) { checkHasSelf(i.target); }
void MemoryObjsCollector::CollectMemVisitor::operator()(minst::Load& i) { checkHasSelf(i.target); }
void MemoryObjsCollector::CollectMemVisitor::operator()(minst::Store& i) {
  checkHasSelf(i.target);
  checkHasSelf(i.value);
}

void MemoryObjsCollector::CollectMemVisitor::operator()(minst::Op& i) {
  if (i.lhs.has_value()) { checkHasSelf(i.lhs.value()); }
  checkHasSelf(i.rhs);
}
void MemoryObjsCollector::CollectMemVisitor::insertAllocaInst(minst::Function& i,
                                                              types::Alias& type) const {
  // i.parent->instructions.insert(std::next(position),
  //                               mir::AllocaInst{{i.lv_name + ".memobj"}, type});
}

void MemoryObjsCollector::CollectMemVisitor::operator()(minst::Fcall& i) {
  std::shared_ptr<FunObjTree> tree;

    if (mir::toString(*i.fname) == "delay") {  // todo:dont comapre with string
      // TODO(tomoya):size should be compile-time constant like below↓
      // int delay_size = eval(i.args[1]);
      tree = std::make_shared<FunObjTree>(FunObjTree{"delay", false, {}, types::delaystruct});
    } else {
      tree = M.traverseFunTree(std::get<minst::Function>(std::get<mir::Instructions>(*i.fname)));
    }
    if (tree->hasself || !tree->memobjs.empty() || mir::toString(*i.fname) == "delay") {
      obj.emplace_back(tree);
      objtype.arg_types.push_back(tree->objtype);
    }
    for (auto& a : i.args) { checkHasSelf(a); }
    if (i.time) { checkHasSelf(i.time.value()); }
  }
  void MemoryObjsCollector::CollectMemVisitor::operator()(minst::MakeClosure& i) {
    //??
  }
  void MemoryObjsCollector::CollectMemVisitor::operator()(minst::Array& i) {
    for (auto&& e : i.args) { checkHasSelf(e); }
  }
  void MemoryObjsCollector::CollectMemVisitor::operator()(minst::Field& i) {
    checkHasSelf(i.target);
    checkHasSelf(i.index);
  }
  void MemoryObjsCollector::CollectMemVisitor::operator()(minst::If& i) {
    checkHasSelf(i.cond);
    for (auto& inst : i.thenblock->instructions) { this->visit(inst); }
    if (i.elseblock.has_value()) {
      for (auto& inst : i.elseblock.value()->instructions) { this->visit(inst); }
    }
  }
  void MemoryObjsCollector::CollectMemVisitor::operator()(minst::Return& i) { checkHasSelf(i.val); }
}  // namespace mimium