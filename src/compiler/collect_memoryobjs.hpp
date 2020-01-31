/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <queue>

#include "basic/mir.hpp"
namespace mimium {

class MemoryObjsCollector {
 public:
  explicit MemoryObjsCollector(TypeEnv& typeenv);
  std::shared_ptr<MIRblock> process(std::shared_ptr<MIRblock> toplevel);
  bool hasMemObj(const std::string& fname){return getAliasFromMap(fname).has_value();};
  auto getMemObjType(const std::string& fname){return getAliasFromMap(fname).value();};
  auto& getMemObjNames(const std::string& fname){return memobjs_map[fname];};

  void dump();
 private:
  TypeEnv& typeenv;
  using type_or_alias = std::variant<std::string, types::Value>;

  std::unordered_map<std::string, std::vector<std::string>> memobjs_map;
  std::unordered_map<std::string, types::Alias> type_alias_map;
  void emplaceNewAlias(std::string& name,types::Value type);
  std::optional<types::Alias> getAliasFromMap(std::string name);

  void collectSelf(std::string& funname, std::string& varname);
  void collectDelay(std::string& funname, int delay_size);
  void collectMemPrim(std::string& funname,std::string& argname);

  void collectMemFun(std::string& funname, std::string& varname);
  struct CollectMemVisitor {
    explicit CollectMemVisitor(MemoryObjsCollector& m) : M(m){};
    MemoryObjsCollector& M;
    std::list<Instructions>::iterator position;
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
    void insertAllocaInst(FunInst& i,types::Alias& type  );
    std::string cur_fun;
  } cm_visitor;
};

}  // namespace mimium