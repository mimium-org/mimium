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
struct instruction {  // base class for MIR instruction
  std::string lv_name;
  blockptr parent = nullptr;
};

struct NumberInst : public instruction {
  double val = 0.0;
};
std::string toString(NumberInst& i);

struct StringInst : public instruction {
  std::string val;
};
std::string toString(StringInst& i);

struct AllocaInst : public instruction {
  types::Value type = types::None();
};
std::string toString(AllocaInst& i);

struct RefInst : public instruction {
  std::string val;
  types::Value type = types::None();
};
std::string toString(RefInst& i);

struct AssignInst : public instruction {
  std::string val;

  types::Value type = types::None();
};
std::string toString(AssignInst& i);

struct OpInst : public instruction {
 public:
  ast::OpId op;
  std::string lhs;
  std::string rhs;
};
std::string toString(OpInst& i);

struct FunInst : public instruction {
  std::deque<std::string> args;
  blockptr body;
  bool hasself = false;
  bool isrecursive = false;
  // introduced after closure conversion;
  // contains self & delay, and fcall which
  // has self&delay
  std::vector<std::string> freevariables = {};
  std::vector<std::string> memory_objects = {};
  bool ccflag = false;  // utility for closure conversion
};
std::string toString(FunInst& i);

struct FcallInst : public instruction {
  std::string fname;
  std::deque<std::string> args;
  FCALLTYPE ftype;
  std::optional<std::string> time;
};
std::string toString(FcallInst& i);

struct MakeClosureInst : public instruction {
  std::string fname;
  std::vector<std::string> captures;
  types::Value capturetype;
};
std::string toString(MakeClosureInst& i);

struct ArrayInst : public instruction {
  std::deque<std::string> args;
};
std::string toString(ArrayInst& i);

struct ArrayAccessInst : public instruction {
  std::string name;
  std::string index;
};
std::string toString(ArrayAccessInst& i);

struct FieldInst : public instruction {
  std::string name;
  std::variant<std::string,int> index;
};
std::string toString(FieldInst& i);

struct IfInst : public instruction {
  std::string cond;
  blockptr thenblock;
  std::optional<blockptr> elseblock;
};
std::string toString(IfInst& i);

struct ReturnInst : public instruction {
  std::string val;
};
std::string toString(ReturnInst& i);
using Instructions = std::variant<NumberInst, StringInst, AllocaInst, RefInst, AssignInst, OpInst,
                                  FunInst, FcallInst, MakeClosureInst, ArrayInst, ArrayAccessInst,
                                  FieldInst, IfInst, ReturnInst>;

inline std::string toString(Instructions& inst) {
  return std::visit([](auto& i) -> std::string { return toString(i); }, inst);
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