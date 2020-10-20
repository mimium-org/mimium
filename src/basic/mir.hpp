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

using valueptr = std::shared_ptr<Value>;
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

struct Load :public Base{
  valueptr target;
};
std::string toString(Load& i);

struct Store :public Base{
  valueptr target;
  valueptr value;
};
std::string toString(Store& i);

struct Op : public Base {
 public:
  ast::OpId op;
  valueptr lhs;
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
  std::variant<std::string, int> index;
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

inline Instructions& addInstToBlock(Instructions&& inst, blockptr block) {
  std::visit([&](auto& i) { i.parent = block->shared_from_this(); }, inst);
  return block->instructions.emplace_back(std::move(inst));
}

inline void addIndentToBlock(blockptr block, int level = 0) { block->indent_level += level; }

}  // namespace mir
}  // namespace mimium