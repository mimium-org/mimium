/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "basic/mir.hpp"

#include "basic/ast_to_string.hpp"

namespace mimium::mir {

std::string toString(const blockptr block) {
  std::stringstream ss;
  for (int i = 0; i < block->indent_level; i++) { ss << "  "; }
  ss << block->label << ":\n";
  for (auto const& inst : block->instructions) {
    for (int i = 0; i < block->indent_level + 1; i++) { ss << "  "; }
    ss << toString(*inst) << "\n";
  }
  return ss.str();
}

std::string instruction::toString(Number const& i) {
  return i.name + " = " + std::to_string(i.val);
}
std::string instruction::toString(String const& i) { return i.name + " = " + i.val; }

std::string instruction::toString(Allocate const& i) {
  return "allocate " + i.name + " : " + types::toString(i.type);
}
std::string instruction::toString(Ref const& i) {
  return i.name + " = ref " + mir::getName(*i.target);
}

std::string instruction::toString(Load const& i) {
  return i.name + " = load " + mir::getName(*i.target);
}
std::string instruction::toString(Store const& i) {
  return "store " + mir::getName(*i.value) + " to " + mir::getName(*i.target);
}

std::string instruction::toString(Op const& i) {
  auto opstr = std::string(ast::op_str.find(i.op)->second);
  return i.name + " = " + opstr + " " + (i.lhs.has_value() ? mir::getName(*i.lhs.value()) : "") +
         " " + mir::getName(*i.rhs);
}

std::string toString(Argument const& i) { return i.name; }

std::string instruction::toString(Function const& i) {
  std::stringstream ss;
  ss << i.name << " = fun" << ((i.isrecursive) ? "[rec]" : "") << " " << join(i.args, " , ");
  if (!i.freevariables.empty()) { ss << " fv{" << join(i.freevariables, ",") << "}"; }
  ss << "\n" << toString(i.body);
  return ss.str();
}

std::string instruction::toString(MakeClosure const& i) {
  std::stringstream ss;
  ss << i.name << " = makeclosure " << mir::getName(*i.fname) << " " << join(i.captures, ",");
  return ss.str();
}
std::string instruction::toString(Fcall const& i) {
  std::string s;
  auto timestr = (i.time) ? "@" + mir::getName(*i.time.value()) : "";
  return i.name + " = app" + fcalltype_str[i.ftype] + " " + mir::getName(*i.fname) + " " +
         join(i.args, " , ") + timestr;
}

std::string instruction::toString(Array const& i) {
  return i.name + " = array " + join(i.args, " , ");
}
std::string instruction::toString(ArrayAccess const& i) {
  std::string res =
      i.name + " = arrayaccess " + mir::getName(*i.target) + " " + mir::getName(*i.index);
  return res;
}

std::string instruction::toString(Field const& i) {
  std::string res = i.name + " = field " + mir::getName(*i.target) + " " + mir::getName(*i.index);
  return res;
}
std::string instruction::toString(If const& i) {
  std::string s;
  s += i.name + " = if " + mir::getName(*i.cond) + "\n";
  s += toString(i.thenblock);
  if (i.elseblock.has_value()) { s += toString(i.elseblock.value()); }
  return s;
}
std::string instruction::toString(Return const& i) { return "return " + mir::getName(*i.val); }
}  // namespace mimium::mir