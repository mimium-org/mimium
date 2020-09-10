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

std::string toString(NumberInst& i) { return i.lv_name + " = " + std::to_string(i.val); }
std::string toString(StringInst& i) { return i.lv_name + " = " + i.val; }

std::string toString(AllocaInst& i) {
  return "alloca: " + i.lv_name + " (" + types::toString(i.type) + ")";
}
std::string toString(RefInst& i) { return i.lv_name + " = ref " + i.val; }

std::string toString(AssignInst& i) { return i.lv_name + " =(overwrite) " + i.val; }

std::string toString(OpInst& i) {
  auto opstr = std::string(ast::op_str.find(i.op)->second);
  return i.lv_name + " = " + opstr + " " + i.lhs + " " + i.rhs;
}

std::string toString(FunInst& i) {
  std::stringstream ss;
  ss << i.lv_name << " = fun" << ((i.isrecursive) ? "[rec]" : "") << " " << join(i.args, " , ");
  if (!i.freevariables.empty()) { ss << " fv{" << joinVec(i.freevariables, ",") << "}"; }
  ss << "\n" << toString(i.body);
  return ss.str();
}

std::string toString(MakeClosureInst& i) {
  std::stringstream ss;
  ss << i.lv_name << " = makeclosure " << i.fname << " " << joinVec(i.captures, ",");
  return ss.str();
}
std::string toString(FcallInst& i) {
  std::string s;
  auto timestr = (i.time) ? "@" + i.time.value() : "";
  return i.lv_name + " = app" + fcalltype_str[i.ftype] + " " + i.fname + " " + join(i.args, " , ") +
         timestr;
}

std::string toString(ArrayInst& i) { return i.lv_name + " = array " + join(i.args, " , "); }

std::string toString(ArrayAccessInst& i) {
  return i.lv_name + " = arrayaccess " + i.name + " " + i.index;
}

std::string toString(IfInst& i) {
  std::string s;
  s += i.lv_name + " = if " + i.cond + "\n";
  s += toString(i.thenblock);
  if (i.elseblock.has_value()) { s += toString(i.elseblock.value()); }
  return s;
}
std::string toString(ReturnInst& i) { return "return " + i.val; }

}  // namespace mimium::mir