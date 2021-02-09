/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <unordered_set>
#include "basic/mir.hpp"
namespace mimium {
namespace minst = mir::instruction;
struct FunObjTree {
  mir::valueptr fname;
  bool hasself = false;
  std::list<std::shared_ptr<FunObjTree>> memobjs;
  types::Value objtype;
};

using funobjmap = std::unordered_map<mir::valueptr, std::shared_ptr<FunObjTree>>;

class MemoryObjsCollector {
 public:
  MemoryObjsCollector() = default;
  funobjmap process(mir::blockptr toplevel);

#ifdef MIMIUM_DEBUG_BUILD
  void dump() const;
  static void dumpFunObjTree(FunObjTree const& tree, int indent = 0);
  FunObjTree dump_res;  // for dump;
#endif
 private:
  std::shared_ptr<FunObjTree> traverseFunTree(mir::valueptr fun);
  static std::string indentHelper(int indent);
  static std::unordered_set<mir::valueptr> collectToplevelFuns(mir::blockptr toplevel);
  static std::optional<mir::valueptr> tryFindFunByName(std::unordered_set<mir::valueptr> fnset,
                                                       std::string const& name);

  funobjmap result_map;

 public:
  struct CollectMemVisitor {
    explicit CollectMemVisitor(MemoryObjsCollector& m) : M(m){};

    MemoryObjsCollector& M;
    struct ResultT {
      std::list<std::shared_ptr<FunObjTree>> objs = {};
      types::Value objtype = types::Alias{"", types::Tuple{}};
      bool hasself = false;
    };

    ResultT operator()(minst::Ref& i);
    ResultT operator()(minst::Load& i);
    ResultT operator()(minst::Store& i);
    ResultT operator()(minst::Op& i);
    ResultT operator()(minst::Fcall& i);
    ResultT operator()(minst::MakeClosure& i);
    ResultT operator()(minst::Array& i);
    ResultT operator()(minst::ArrayAccess& i);
    ResultT operator()(minst::Field& i);
    ResultT operator()(minst::If& i);
    ResultT operator()(minst::Return& i);
    template <typename T>
    ResultT operator()(T& /*i*/) {
      constexpr bool isfun = std::is_same_v<minst::Function, std::decay_t<T>>;
      if constexpr (isfun) {
        assert(false && "Memobj collector reached to FunInst. maybe failed to Closure Conversion?");
      } else {
        static_assert(std::is_base_of_v<mir::instruction::Base, std::decay_t<T>>,
                      "mir instruction unreachable");
        // do nothing for primitive type
      }
      return ResultT{};
    }
    ResultT visit(mir::valueptr v) {
      if (auto* instptr = std::get_if<mir::Instructions>(v.get())) {
        return std::visit(*this, *instptr);
      } else {
        assert(false);
      }
      return ResultT{};
    }
    ResultT visitInsts(mir::blockptr block);
    static types::Tuple& getTupleFromAlias(types::Value& t);

   private:
    static bool isSelf(mir::valueptr val) { return std::holds_alternative<mir::Self>(*val); };
    static bool isExternalFunMemobj(const mir::ExternalSymbol& s) {
      return s.name == "delay" || s.name == "mem";
    }
    static ResultT makeResfromHasSelf(bool hasself);
    static void mergeResultTs(ResultT& dest, ResultT& src);
  };
};

}  // namespace mimium