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
  for (auto& inst : block->instructions) {
    for (int i = 0; i < block->indent_level + 1; i++) { ss << "  "; }
    ss << toString(inst) << "\n";
  }
  return ss.str();
}

std::string toString(Value const& v) { return v.name; }

namespace instruction {
std::string toString(Number& i) {
  return toString(i.lv_name.value()) + " = " + std::to_string(i.val);
}
std::string toString(String& i) { return toString(i.lv_name.value()) + " = " + i.val; }

std::string toString(Allocate& i) {
  return "alloca: " + toString(i.lv_name.value()) + " (" + types::toString(i.type) + ")";
}
std::string toString(Ref& i) {
  return toString(i.lv_name.value()) + " = ref " + toString(*i.target);
}

std::string toString(Load& i) {
  return toString(i.lv_name.value()) + "= load " + toString(*i.target);
}
std::string toString(Store& i) {
  return toString(*i.target) + " = " + toString(*i.value) + "(store)";
}

std::string toString(Op& i) {
  auto opstr = std::string(ast::op_str.find(i.op)->second);
  return toString(i.lv_name.value()) + " = " + opstr + " " +
         (i.lhs.has_value() ? toString(*i.lhs.value()) : "") + " " + toString(*i.rhs);
}

std::string toString(Function& i) {
  std::stringstream ss;
  ss << toString(i.lv_name.value()) << " = fun" << ((i.isrecursive) ? "[rec]" : "") << " "
     << join(i.args, " , ");
  if (!i.freevariables.empty()) { ss << " fv{" << joinVec(i.freevariables, ",") << "}"; }
  ss << "\n" << toString(i.body);
  return ss.str();
}

std::string toString(MakeClosure& i) {
  std::stringstream ss;
  ss << toString(i.lv_name.value()) << " = makeclosure " << toString(*i.fname) << " "
     << joinVec(i.captures, ",");
  return ss.str();
}
std::string toString(Fcall& i) {
  std::string s;
  auto timestr = (i.time) ? "@" + toString(*i.time.value()) : "";
  return toString(i.lv_name.value()) + " = app" + fcalltype_str[i.ftype] + " " +
         toString(*i.fname) + " " + join(i.args, " , ") + timestr;
}

std::string toString(Array& i) {
  return toString(i.lv_name.value()) + " = array " + join(i.args, " , ");
}

std::string toString(Field& i) {
  std::string res = toString(i.lv_name.value()) + " = field " + toString(*i.name) + " ";
  std::visit(
      overloaded{[&](std::string& s) { res += s; }, [&](int i) { res += std::to_string(i); }},
      i.index);
  return res;
}
std::string toString(If& i) {
  std::string s;
  s += toString(i.lv_name.value()) + " = if " + toString(*i.cond) + "\n";
  s += toString(i.thenblock);
  if (i.elseblock.has_value()) { s += toString(i.elseblock.value()); }
  return s;
}
std::string toString(Return& i) { return "return " + toString(*i.val); }
}  // namespace instruction
}  // namespace mimium::mir