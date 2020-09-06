/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/mir.hpp"
namespace mimium {

class MemoryObjsCollector {
 public:
  explicit MemoryObjsCollector(TypeEnv& typeenv);
  mir::blockptr process(mir::blockptr toplevel);
  bool hasMemObj(const std::string& fname) {
    return getAliasFromMap(fname).has_value();
  };
  auto getMemObjType(const std::string& fname) {
    return getAliasFromMap(fname).value();
  };
  auto& getMemObjNames(const std::string& fname) { return memobjs_map[fname]; };

  void dump();

 private:
  TypeEnv& typeenv;
  using type_or_alias = std::variant<std::string, types::Value>;

  std::unordered_map<std::string, std::vector<std::string>> memobjs_map;
  std::unordered_map<std::string, types::Alias> type_alias_map;
  void emplaceNewAlias(std::string& name, types::Value type);
  std::optional<types::Alias> getAliasFromMap(std::string name);

  void collectSelf(std::string& funname, std::string& varname);
  void collectDelay(std::string& funname, int delay_size);
  void collectMemPrim(std::string& funname, std::string& argname);

  void collectMemFun(std::string& funname, std::string& varname);
  struct CollectMemVisitor {
    explicit CollectMemVisitor(MemoryObjsCollector& m) : M(m){};
    MemoryObjsCollector& M;
    std::list<mir::Instructions>::iterator position;

    void operator()(mir::RefInst& i);
    void operator()(mir::AssignInst& i);
    void operator()(mir::OpInst& i);
    void operator()(mir::FunInst& i);
    void operator()(mir::FcallInst& i);
    void operator()(mir::MakeClosureInst& i);
    void operator()(mir::ArrayInst& i);
    void operator()(mir::ArrayAccessInst& i);
    void operator()(mir::IfInst& i);
    void operator()(mir::ReturnInst& i);
    template<typename T>
    void operator()(T& /*i*/){
      if constexpr(std::is_base_of_v<mir::instruction, std::decay_t<T>>){
        // do nothing
      }else{
        static_assert(true, "mir instruction unreachable");
      }
    }
   private:
    void insertAllocaInst(mir::FunInst& i, types::Alias& type)const;
    std::string cur_fun;
  } cm_visitor;
};

}  // namespace mimium