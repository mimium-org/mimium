#include "collect_memoryobjs.hpp"
namespace mimium {

MemoryObjsCollector::MemoryObjsCollector(TypeEnv& typeenv)
    : typeenv(typeenv), cm_visitor(*this) {}

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
  return toplevel;
}
void MemoryObjsCollector::emplaceNewAlias(std::string& name,
                                          types::Value type) {
  type_alias_map.emplace(name, types::Alias(name, type));
}

types::Alias& MemoryObjsCollector::getAliasFromMap(std::string name) {
  auto it = type_alias_map.find(name);
  if (it == type_alias_map.end()) {
    throw std::runtime_error("could not find alias type for name \"" + name +
                             "\"");
  }
  return it->second;
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
}
void MemoryObjsCollector::collectDelay(std::string& funname, int delay_size) {
  auto delayname = "delay." + std::to_string(delay_size);
  emplaceNewAlias(delayname, types::Array(types::Float(), delay_size));
  memobjs_map[funname].emplace_back(delayname);
}
void MemoryObjsCollector::collectMemFun(std::string& funname,
                                        std::string& varname) {
  auto it = memobjs_map.find(varname);
  if (it != memobjs_map.end()) {
    memobjs_map[funname].emplace_back(varname);
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
  this->cur_fun = i.lv_name;
  for (auto& inst : *i.body) {
    std::visit(*this, inst);
  }

  std::vector<types::Value> objs;
  for (auto& alias_name : M.memobjs_map[i.lv_name]) {
    objs.emplace_back(M.getAliasFromMap(alias_name));
  }
  auto type = types::Alias(i.lv_name + ".mem", types::Tuple(std::move(objs)));
  
  if(i.lv_name == "dsp"){
    insertAllocaInst(i,type);
  }
  M.type_alias_map.emplace(i.lv_name, std::move(type));
}
void MemoryObjsCollector::CollectMemVisitor::insertAllocaInst(FunInst& i,types::Alias& type){
  i.parent->instructions.insert(std::next(position),AllocaInst(i.lv_name+".mem", type));
}

void MemoryObjsCollector::CollectMemVisitor::operator()(FcallInst& i) {
  if (i.fname == "delay") {
    // how to track delay size in ssa !!!
    // M.collectDelay(cur_fun, int delay_size)
  } else {
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