#pragma once
#include <queue>

#include "basic/mir.hpp"
namespace mimium {

class MemoryObjsCollector {
 public:
  explicit MemoryObjsCollector(TypeEnv& typeenv);
  std::shared_ptr<MIRblock> process(std::shared_ptr<MIRblock> toplevel);
  types::Alias& getAliasFromMap(std::string name);
  void dump();
 private:
  TypeEnv& typeenv;
  using type_or_alias = std::variant<std::string, types::Value>;

  std::unordered_map<std::string, std::vector<std::string>> memobjs_map;
  std::unordered_map<std::string, types::Alias> type_alias_map;
  void emplaceNewAlias(std::string& name,types::Value type);
  void collectSelf(std::string& funname, std::string& varname);
  void collectDelay(std::string& funname, int delay_size);
  void collectMemFun(std::string& funname, std::string& varname);
  struct CollectMemVisitor {
    explicit CollectMemVisitor(MemoryObjsCollector& m) : M(m){};
    MemoryObjsCollector& M;
    void operator()(NumberInst& i);
    void operator()(AllocaInst& i);
    void operator()(RefInst& i);
    void operator()(AssignInst& i);
    void operator()(TimeInst& i);
    void operator()(OpInst& i);
    void operator()(FunInst& i);
    void operator()(FcallInst& i);
    void operator()(MakeClosureInst& i);
    void operator()(ArrayInst& i);
    void operator()(ArrayAccessInst& i);
    void operator()(IfInst& i);
    void operator()(ReturnInst& i);

    private:
    std::string cur_fun;
  } cm_visitor;
};

}  // namespace mimium