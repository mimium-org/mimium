/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <list>
#include <utility>

#include "basic/ast.hpp"
#include "basic/type.hpp"

namespace mimium {

enum FCALLTYPE { DIRECT, CLOSURE, EXTERNAL };
static std::map<FCALLTYPE, std::string> fcalltype_str = {
    {DIRECT, ""}, {CLOSURE, "cls"}, {EXTERNAL, "ext"}};

namespace mir {

class block;
using blockptr = std::shared_ptr<block>;

struct Value {
  std::string name;
  types::Value type;
};

std::string toString(Value const&);

using valueptr = Value*;
namespace instruction {
struct Base {  // base class for MIR instruction
  std::optional<Value> lv_name;
  blockptr parent = nullptr;
};

struct Number : public Base {
  double val = 0.0;
};
std::string toString(Number& i);

struct String : public Base {
  std::string val;
};
std::string toString(String& i);

struct Allocate : public Base {
  types::Value type = types::None();
};
std::string toString(Allocate& i);

struct Ref : public Base {
  valueptr target;
};
std::string toString(Ref& i);

struct Load : public Base {
  valueptr target;
};
std::string toString(Load& i);

// lv_name should be discarded for Store
struct Store : public Base {
  valueptr target;
  valueptr value;
};
std::string toString(Store& i);

struct Op : public Base {
 public:
  ast::OpId op;
  std::optional<valueptr> lhs;
  valueptr rhs;
};
std::string toString(Op& i);

struct Function : public Base {
  std::deque<valueptr> args;
  blockptr body;
  bool hasself = false;
  bool isrecursive = false;
  // introduced after closure conversion;
  // contains self & delay, and fcall which
  // has self&delay
  std::vector<valueptr> freevariables = {};
  std::vector<valueptr> memory_objects = {};
  bool ccflag = false;  // utility for closure conversion
};
std::string toString(Function& i);

struct Fcall : public Base {
  valueptr fname;
  std::deque<valueptr> args;
  FCALLTYPE ftype;
  std::optional<valueptr> time;
};
std::string toString(Fcall& i);

struct MakeClosure : public Base {
  valueptr fname;
  std::vector<valueptr> captures;
  types::Value capturetype;
};
std::string toString(MakeClosure& i);

struct Array : public Base {
  std::deque<valueptr> args;
};
std::string toString(Array& i);

struct Field : public Base {
  valueptr name;
  std::variant<valueptr, int> index;
};
std::string toString(Field& i);

struct If : public Base {
  valueptr cond;
  blockptr thenblock;
  std::optional<blockptr> elseblock;
};
std::string toString(If& i);

struct Return : public Base {
  valueptr val;
};

std::string toString(Return& i);
using Instructions = std::variant<Number, String, Allocate, Ref, Load, Store, Op, Function, Fcall,
                                  MakeClosure, Array, Field, If, Return>;
}  // namespace instruction

inline std::string join(std::deque<valueptr>& vec, std::string delim) {
  std::string res;
  if (!vec.empty()) {
    res = std::accumulate(
        std::next(vec.begin()), vec.end(), toString((**vec.begin())),
        [&](std::string a, valueptr& b) { return std::move(a) + delim + toString(*b); });
  }
  return res;
};

template <typename T, typename... Ts>
valueptr makeInst(std::string lv_name, types::Value type, blockptr parent, Ts... args) {
  return T{mir::instruction::Base{{std::optional(Value{lv_name, type})}}, args...};
}

using Instructions = instruction::Instructions;
inline std::string toString(Instructions& inst) {
  return std::visit([](auto& i) -> std::string { return instruction::toString(i); }, inst);
}

class block : public std::enable_shared_from_this<block> {
 public:
  std::string label;
  std::list<Instructions> instructions;  // sequence of instructions
  int indent_level = 0;                  // shared between instances
};

inline blockptr makeBlock(std::string const& label, int indent = 0) {
  auto b = std::make_shared<block>();
  b->label = label;
  b->indent_level = indent;
  return b;
}

std::string toString(blockptr block);

inline auto addInstToBlock(Instructions&& inst, blockptr block) {
  std::visit([&](auto& i) { i.parent = block; }, inst);
  std::optional<valueptr> ret = std::nullopt;
  std::visit(
      [&](auto& i) {
        if (i.lv_name.has_value()) { ret = &i.lv_name.value(); }
      },
      inst);
  block->instructions.emplace_back(std::move(inst));
  return ret;
}

inline void addIndentToBlock(blockptr block, int level = 0) { block->indent_level += level; }

}  // namespace mir
}  // namespace mimium