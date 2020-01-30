/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "collect_memoryobjs.hpp"
namespace mimium {

MemoryObjsCollector::MemoryObjsCollector(TypeEnv& typeenv)
    : typeenv(typeenv), cm_visitor(*this) {
      std::string primitivemem = "mem";
      memobjs_map.emplace(primitivemem,std::vector<std::string>{"memory"});
      emplaceNewAlias(primitivemem,types::Float());
    }

std::shared_ptr<MIRblock> MemoryObjsCollector::process(
    std::shared_ptr<MIRblock> toplevel) {
  auto it = std::begin(*toplevel);
  for (Instructions& inst : *toplevel) {
    // skip global declaration
    if (std::holds_alternative<FunInst>(inst)) {
      cm_visitor.position = it;
      std::visit(cm_visitor, inst);
    }
    ++it;
  }
  // register to typeenv
  for (auto&& [name, val] : type_alias_map) {
    typeenv.emplace(val.name + "obj", val);
  }
  return toplevel;
}
void MemoryObjsCollector::emplaceNewAlias(std::string& name,
                                          types::Value type) {
  type_alias_map.emplace(name, types::Alias(name, type));
}

std::optional<types::Alias> MemoryObjsCollector::getAliasFromMap(
    std::string name) {
  auto it = type_alias_map.find(name);
  std::optional<types::Alias> res;
  if (it != type_alias_map.end()) {
    res = it->second;
  }
  return res;
}
void MemoryObjsCollector::collectSelf(std::string& funname,
                                      std::string& varname) {
  auto newname = (funname + ".self");
  bool isself = (varname == "self");
  // self is counted only once in 1 function.
  bool ischecked = type_alias_map.count(newname) > 0;
  if (isself && !ischecked) {
    auto& ftype = rv::get<types::Function>(typeenv.find(funname));
    emplaceNewAlias(newname, ftype.ret_type);
    memobjs_map[funname].emplace_back(newname);
  }
  if (isself) {
    // destructive: rewrite name of "self"
    varname = newname + ".mem";
  }
}
void MemoryObjsCollector::collectDelay(std::string& funname, int delay_size) {
  auto delayname = "delay." + std::to_string(delay_size);
  emplaceNewAlias(delayname, types::Array(types::Float(), delay_size));
  memobjs_map[funname].emplace_back(delayname);
}
void MemoryObjsCollector::collectMemPrim(std::string& funname,std::string& argname) {
  // auto newname = (funname + ".memprim");
  // bool ismemprim = (varname == "mem");
  // bool ischecked = type_alias_map.count(newname) > 0;
  //  if (ismemprim && !ischecked) {
  //   emplaceNewAlias(newname, typeenv.find(funname));
  //   memobjs_map[funname].emplace_back(newname);
  // }
  // if (ismemprim) {
  //   // destructive: rewrite name of "self"
  //   varname = newname + ".mem";
  // }
}

void MemoryObjsCollector::collectMemFun(std::string& funname,
                                        std::string& varname) {
  auto it = memobjs_map.find(varname);
  if (it != memobjs_map.end()) {
    if (!it->second.empty()) {
      memobjs_map[funname].emplace_back(varname);
    }
  }
}

void MemoryObjsCollector::dump() {
  std::cerr << "-------------Memory Object Functions: -----\n";
  for (auto&& [key, val] : memobjs_map) {
    std::cerr << key << " : ";
    std::for_each(val.begin(), val.end(),
                  [](auto& s) { std::cerr << s << " "; });
    std::cerr << "\n";
  }
  std::cerr << "-------------type alias Map: -----\n";
  for (auto&& [key, val] : type_alias_map) {
    std::cerr << key << " : " << types::toString(val, true) << "\n";
  }
  std::cerr << "------------\n";
}

// visitor
void MemoryObjsCollector::CollectMemVisitor::operator()(NumberInst& i) {
  // do nothing
}
void MemoryObjsCollector::CollectMemVisitor::operator()(AllocaInst& i) {
  // do nothing
}
void MemoryObjsCollector::CollectMemVisitor::operator()(RefInst& i) {
  M.collectSelf(cur_fun, i.val);
}
void MemoryObjsCollector::CollectMemVisitor::operator()(AssignInst& i) {
  M.collectSelf(cur_fun, i.val);
}
void MemoryObjsCollector::CollectMemVisitor::operator()(TimeInst& i) {
  M.collectSelf(cur_fun, i.time);
  M.collectSelf(cur_fun, i.val);
}
void MemoryObjsCollector::CollectMemVisitor::operator()(OpInst& i) {
  M.collectSelf(cur_fun, i.lhs);
  M.collectSelf(cur_fun, i.rhs);
}
void MemoryObjsCollector::CollectMemVisitor::operator()(FunInst& i) {
  auto memname = i.lv_name + ".mem";
  this->cur_fun = i.lv_name;
  for (auto& inst : *i.body) {
    std::visit(*this, inst);
  }
  auto result = M.memobjs_map[i.lv_name];
  if (!result.empty()) {
    std::vector<types::Value> objs;
    for (auto& alias_name : result) {
      objs.emplace_back(M.getAliasFromMap(alias_name).value());
    }
    auto type = types::Alias(memname, types::Tuple(std::move(objs)));

    if (i.lv_name == "dsp") {
      insertAllocaInst(i, type);
    }
    M.type_alias_map.emplace(i.lv_name, std::move(type));
    i.args.push_back(memname);
    i.memory_objects = result;
    i.hasself = true;
  }
}
void MemoryObjsCollector::CollectMemVisitor::insertAllocaInst(
    FunInst& i, types::Alias& type) {
  i.parent->instructions.insert(std::next(position),
                                AllocaInst(i.lv_name + ".memobj", type));
}

void MemoryObjsCollector::CollectMemVisitor::operator()(FcallInst& i) {
  if (i.fname == "delay") {
    // how to track delay size in ssa !!!
    // M.collectDelay(cur_fun, int delay_size)
  }else {
    M.collectMemFun(cur_fun, i.fname);
  }
  for (auto& a : i.args) {
    M.collectSelf(cur_fun, a);
  }
}
void MemoryObjsCollector::CollectMemVisitor::operator()(MakeClosureInst& i) {
  //??
}
void MemoryObjsCollector::CollectMemVisitor::operator()(ArrayInst& i) {}
void MemoryObjsCollector::CollectMemVisitor::operator()(ArrayAccessInst& i) {
  M.collectSelf(cur_fun, i.name);
  M.collectSelf(cur_fun, i.index);
}
void MemoryObjsCollector::CollectMemVisitor::operator()(IfInst& i) {}
void MemoryObjsCollector::CollectMemVisitor::operator()(ReturnInst& i) {
  M.collectSelf(cur_fun, i.val);
}
}  // namespace mimium