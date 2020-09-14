/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/mir.hpp"
namespace mimium {

struct FunObjTree {
  std::string fname;
  bool hasself = false;
  std::deque<Box<FunObjTree>> memobjs;
  types::Value objtype;
};
using funmap = std::unordered_map<std::string, mir::FunInst*>;
using funobjmap = std::unordered_map<std::string, std::shared_ptr<FunObjTree>>;

class MemoryObjsCollector {
 public:
  explicit MemoryObjsCollector(TypeEnv& typeenv);
  funobjmap& process(mir::blockptr toplevel);

  void dump();

 private:
  TypeEnv& typeenv;
  FunObjTree res;  // for dump;
  static std::string indentHelper(int indent);
  static void dumpFunObjTree(FunObjTree const& tree, int indent = 0);

  static funmap collectToplevelFuns(mir::blockptr toplevel);
  funmap toplevel_funmap;
  funobjmap result_map;
  FunObjTree& traverseFunTree(mir::FunInst const& f);
  struct CollectMemVisitor {
    explicit CollectMemVisitor(MemoryObjsCollector& m,mir::FunInst const& f) : M(m),fun(f){};
    MemoryObjsCollector& M;
    const mir::FunInst& fun;
    std::deque<Box<FunObjTree>> obj = {};
    bool res_hasself = false;
    types::Tuple objtype;

    void operator()(mir::RefInst& i);
    void operator()(mir::AssignInst& i);
    void operator()(mir::OpInst& i);
    void operator()(mir::FcallInst& i);
    void operator()(mir::MakeClosureInst& i);
    void operator()(mir::ArrayInst& i);
    void operator()(mir::ArrayAccessInst& i);
    void operator()(mir::IfInst& i);
    void operator()(mir::ReturnInst& i);
    template <typename T>
    void operator()(T& /*i*/) {
      constexpr bool isfun = std::is_same_v<mir::FunInst, std::decay_t<T>>;
      if constexpr (isfun) {
        // Memobj collector reached to FunInst. maybe failed to Closure Conversion?
        assert(!isfun);
      } else {
        static_assert(std::is_base_of_v<mir::instruction, std::decay_t<T>>,
                      "mir instruction unreachable");
      }
    }

   private:
    void insertAllocaInst(mir::FunInst& i, types::Alias& type) const;
    void checkHasSelf(std::string const& name) {
      res_hasself |= name == "self";
    }
    std::string cur_fun;
  };
};

}  // namespace mimium