/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/mir.hpp"
namespace mimium {
namespace minst = mir::instruction;

struct FunObjTree {
  std::string fname;
  bool hasself = false;
  std::deque<Box<FunObjTree>> memobjs;
  types::Value objtype;
};
using funmap = std::unordered_map<std::string, minst::Function*>;
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
  FunObjTree& traverseFunTree(minst::Function const& f);
  struct CollectMemVisitor {
    explicit CollectMemVisitor(MemoryObjsCollector& m,minst::Function const& f) : M(m),fun(f){};
    MemoryObjsCollector& M;
    const minst::Function& fun;
    std::deque<Box<FunObjTree>> obj = {};
    bool res_hasself = false;
    types::Tuple objtype;


    void operator()(minst::Ref& i);
    void operator()(minst::Load& i);
    void operator()(minst::Store& i);
    void operator()(minst::Op& i);
    void operator()(minst::Fcall& i);
    void operator()(minst::MakeClosure& i);
    void operator()(minst::Array& i);
    void operator()(minst::Field& i);
    void operator()(minst::If& i);
    void operator()(minst::Return& i);
    template <typename T>
    void operator()(T& /*i*/) {
      constexpr bool isfun = std::is_same_v<minst::Function, std::decay_t<T>>;
      if constexpr (isfun) {
        // Memobj collector reached to FunInst. maybe failed to Closure Conversion?
        assert(!isfun);
      } else {
        static_assert(std::is_base_of_v<mir::instruction::Base, std::decay_t<T>>,
                      "mir instruction unreachable");
      }
    }

   private:
    void insertAllocaInst(minst::Function& i, types::Alias& type) const;
    void checkHasSelf(std::string const& name) {
      res_hasself |= name == "self";
    }
    std::string cur_fun;
  };
};

}  // namespace mimium