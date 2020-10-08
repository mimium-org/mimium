/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "collect_memoryobjs.hpp"
namespace mimium {

MemoryObjsCollector::MemoryObjsCollector(TypeEnv& typeenv) : typeenv(typeenv) {}

funmap MemoryObjsCollector::collectToplevelFuns(mir::blockptr toplevel) {
  funmap res;
  for (mir::Instructions& inst : toplevel->instructions) {
    if (auto* funptr = std::get_if<mir::FunInst>(&inst)) { res.emplace(funptr->lv_name, funptr); }
  }
  return res;
}

FunObjTree& MemoryObjsCollector::traverseFunTree(mir::FunInst const& f) {
  std::string name = f.lv_name;
  CollectMemVisitor visitor(*this, f);
  for (auto&& i : f.body->instructions) { std::visit(visitor, i); }
  if (visitor.res_hasself) {
    auto fntype = typeenv.find(f.lv_name);
    auto rettype = rv::get<types::Function>(fntype).ret_type;
    visitor.objtype.arg_types.emplace_back(rettype);
  }
  auto objptr = std::make_shared<FunObjTree>();
  *objptr = FunObjTree{name, visitor.res_hasself, visitor.obj, visitor.objtype};
  result_map.emplace(name, objptr);
  return *objptr;
}

funobjmap& MemoryObjsCollector::process(mir::blockptr toplevel) {
  result_map.emplace(
      "delay", std::make_shared<FunObjTree>(FunObjTree{"delay", false, {}, types::delaystruct}));
  this->toplevel_funmap = collectToplevelFuns(toplevel);
  if (toplevel_funmap.count("dsp") > 0) {
    auto* dspfun = toplevel_funmap.at("dsp");
    auto res = traverseFunTree(*dspfun);
    auto& insts = toplevel->instructions;
    auto dspmemtype = res.objtype;
    auto alloca = mir::AllocaInst{{"dsp.mem"}, dspmemtype};
    insts.insert(std::begin(insts), alloca);
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
    dumpFunObjTree(o, fn_s.size());
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

void MemoryObjsCollector::CollectMemVisitor::operator()(mir::RefInst& i) { checkHasSelf(i.val); }
void MemoryObjsCollector::CollectMemVisitor::operator()(mir::AssignInst& i) { checkHasSelf(i.val); }

void MemoryObjsCollector::CollectMemVisitor::operator()(mir::OpInst& i) {
  checkHasSelf(i.lhs);
  checkHasSelf(i.rhs);
}
void MemoryObjsCollector::CollectMemVisitor::insertAllocaInst(mir::FunInst& i,
                                                              types::Alias& type) const {
  // i.parent->instructions.insert(std::next(position),
  //                               mir::AllocaInst{{i.lv_name + ".memobj"}, type});
}

void MemoryObjsCollector::CollectMemVisitor::operator()(mir::FcallInst& i) {
  FunObjTree tree;
  if (M.toplevel_funmap.count(i.fname) > 0) {
    // TODO(tomoya) create and use cache here
    auto* fun = M.toplevel_funmap.at(i.fname);
    tree = M.traverseFunTree(*fun);
  } else if (i.fname == "delay") {
    // TODO(tomoya):size should be compile-time constant like below↓
    // int delay_size = eval(i.args[1]);
    tree = FunObjTree{"delay", false, {}, types::delaystruct};
  }

  if (tree.hasself || !tree.memobjs.empty() || i.fname == "delay") {
    obj.emplace_back(tree);
    objtype.arg_types.push_back(tree.objtype);
  }
  for (auto& a : i.args) { checkHasSelf(a); }
  if (i.time) { checkHasSelf(i.time.value()); }
}
void MemoryObjsCollector::CollectMemVisitor::operator()(mir::MakeClosureInst& i) {
  //??
}
void MemoryObjsCollector::CollectMemVisitor::operator()(mir::ArrayInst& i) {
  for (auto&& e : i.args) { checkHasSelf(e); }
}
void MemoryObjsCollector::CollectMemVisitor::operator()(mir::ArrayAccessInst& i) {
  checkHasSelf(i.name);
  checkHasSelf(i.index);
}
void MemoryObjsCollector::CollectMemVisitor::operator()(mir::FieldInst& i) {
  checkHasSelf(i.name);
  if(auto* iname = std::get_if<std::string>(&i.index)){
  checkHasSelf(*iname);
  }
}
void MemoryObjsCollector::CollectMemVisitor::operator()(mir::IfInst& i) {
  checkHasSelf(i.cond);
  for (auto& inst : i.thenblock->instructions) { std::visit(*this, inst); }
  if (i.elseblock.has_value()) {
    for (auto& inst : i.elseblock.value()->instructions) { std::visit(*this, inst); }
  }
}
void MemoryObjsCollector::CollectMemVisitor::operator()(mir::ReturnInst& i) { checkHasSelf(i.val); }
}  // namespace mimium