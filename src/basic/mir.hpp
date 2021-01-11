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

namespace instruction {

struct Number;
struct String;
struct Allocate;
struct Ref;
struct Load;
struct Store;
struct Op;
struct Function;
struct Fcall;
struct MakeClosure;
struct Array;
struct ArrayAccess;
struct Field;
struct If;
struct Return;
using Instructions = std::variant<Number, String, Allocate, Ref, Load, Store, Op, Function, Fcall,
                                  MakeClosure, Array, ArrayAccess, Field, If, Return>;
}  // namespace instruction
struct Argument;
// todo: more specific value
struct ExternalSymbol {
  std::string name;
  types::Value type;
};

struct Self;
using Constants = std::variant<int, double, std::string>;

using Value = std::variant<instruction::Instructions, Constants, ExternalSymbol,
                           std::shared_ptr<Argument>, Self>;

using valueptr = std::shared_ptr<Value>;
struct Self {
  valueptr fn;
  types::Value type;
};
struct Argument {
  std::string name;
  types::Value type;
  valueptr parentfn;
};

std::string toString(Argument const& i);

struct FnArgs {
  std::optional<std::shared_ptr<Argument>> ret_ptr;
  std::list<std::shared_ptr<Argument>> args;
};

namespace instruction {
struct Base {  // base class for MIR instruction
  std::string name;
  types::Value type;
  blockptr parent = nullptr;
};

struct Number : public Base {
  double val = 0.0;
};
std::string toString(Number const& i);

struct String : public Base {
  std::string val;
};
std::string toString(String const& i);

struct Allocate : public Base {};
std::string toString(Allocate const& i);

struct Ref : public Base {
  valueptr target;
};
std::string toString(Ref const& i);

struct Load : public Base {
  valueptr target;
};
std::string toString(Load const& i);

// lv_name should be discarded for Store
struct Store : public Base {
  valueptr target;
  valueptr value;
};
std::string toString(Store const& i);

struct Op : public Base {
 public:
  ast::OpId op;
  std::optional<valueptr> lhs;
  valueptr rhs;
};
std::string toString(Op const& i);

struct Function : public Base {
  FnArgs args;
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
std::string toString(Function const& i);

struct Fcall : public Base {
  valueptr fname;
  std::list<valueptr> args;
  FCALLTYPE ftype;
  std::optional<valueptr> time;
};
std::string toString(Fcall const& i);

struct MakeClosure : public Base {
  valueptr fname;
  std::vector<valueptr> captures;
};
std::string toString(MakeClosure const& i);

struct Array : public Base {
  std::vector<valueptr> args;
};
std::string toString(Array const& i);

struct ArrayAccess : public Base {
  valueptr target;
  valueptr index;
};
std::string toString(ArrayAccess const& i);

struct Field : public Base {
  valueptr target;
  valueptr index;
};
std::string toString(Field const& i);

struct If : public Base {
  valueptr cond;
  blockptr thenblock;
  std::optional<blockptr> elseblock;
};
std::string toString(If const& i);

struct Return : public Base {
  valueptr val;
};

std::string toString(Return const& i);

}  // namespace instruction

using Instructions = instruction::Instructions;
inline std::string toString(Instructions const& inst) {
  return std::visit([](auto const& i) -> std::string { return instruction::toString(i); }, inst);
}
inline std::string toString(Constants const& inst) {
  return std::visit(overloaded{[](std::string const& s) -> std::string { return s; },
                               [](const auto i) { return std::to_string(i); }},
                    inst);
}
inline std::string toString(Value const& inst) {
  return std::visit(overloaded{[](Instructions const& i) { return toString(i); },
                               [](Constants const& i) { return toString(i); },
                               [](std::shared_ptr<Argument> i) { return toString(*i); },
                               [](Self const& /*i*/) -> std::string { return "self"; },
                               [](auto const& i) { return i.name; }},
                    inst);
}

inline std::string getName(Instructions const& inst) {
  return std::visit([](auto const& i) { return i.name; }, inst);
}
inline std::string getName(Argument const& i) { return toString(i); };

inline std::string getName(Value const& val) {
  return std::visit(overloaded{[](Instructions const& i) { return getName(i); },
                               [](Constants const& i) { return toString(i); },
                               [](std::shared_ptr<Argument> const& i) { return getName(*i); },
                               [](Self const& /*i*/) -> std::string { return "self"; },
                               [](auto const& i) { return i.name; }},
                    val);
}

template <typename T>
inline std::string join(T const& vec, std::string const& delim) {
  using elemtype = std::decay_t<decltype(*vec.begin())>;
  static_assert(std::is_same_v<elemtype, valueptr> ||
                std::is_same_v<elemtype, std::shared_ptr<Argument>>);
  std::string res;
  if (!vec.empty()) {
    res = std::accumulate(
        std::next(vec.begin()), vec.end(), getName(**vec.begin()),
        [&](std::string const& a, elemtype const& b) { return a + delim + getName(*b); });
  }
  return res;
};

class block : public std::enable_shared_from_this<block> {
 public:
  std::optional<valueptr> parent = std::nullopt;
  std::string label;
  std::list<valueptr> instructions;  // sequence of instructions
  int indent_level = 0;              // shared between instances
};

inline blockptr makeBlock(std::string const& label, int indent = 0) {
  auto b = std::make_shared<block>();
  b->label = label;
  b->indent_level = indent;
  return b;
}

std::string toString(blockptr block);

inline std::shared_ptr<mir::Value> addInstToBlock(Instructions&& inst, blockptr block) {
  auto ptr = std::make_shared<Value>(std::move(inst));
  std::visit([&](auto& i) mutable { i.parent = block; }, std::get<Instructions>(*ptr));
  block->instructions.emplace_back(ptr);
  return ptr;
}

inline void addIndentToBlock(blockptr block, int level = 0) { block->indent_level += level; }

inline bool isConstant(Value const& v) { return std::holds_alternative<Constants>(v); }

inline blockptr getParent(Instructions const& v) {
  return std::visit([](auto const& i) { return i.parent; }, v);
}

inline types::Value getType(Constants const& v) {
  return std::visit(
      overloaded{[](std::string const& /*s*/) -> types::Value { return types::String{}; },
                 [](const double /*i*/) -> types::Value { return types::Float{}; }},
      v);
}
inline types::Value getType(Instructions const& v) {
  return std::visit([](auto const& i) { return i.type; }, v);
}
inline types::Value getType(Value const& v) {
  return std::visit(overloaded{[](Instructions const& i) { return getType(i); },
                               [](Constants const& i) { return getType(i); },
                               [](std::shared_ptr<Argument> i) { return i->type; },
                               [](auto const& i) { return i.type; }},
                    v);
}

template <class ITYPE>
ITYPE& getInstRef(valueptr ptr) {
  return std::get<ITYPE>(std::get<Instructions>(*ptr));
}

template <class ITYPE>
bool isInstA(valueptr ptr) {
  if (std::holds_alternative<Instructions>(*ptr)) {
    return std::holds_alternative<ITYPE>(std::get<Instructions>(*ptr));
  }
  return false;
}

}  // namespace mir
}  // namespace mimium