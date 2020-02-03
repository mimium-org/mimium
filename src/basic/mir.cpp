/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
 
#include "basic/mir.hpp"

namespace mimium {

std::string MIRblock::toString() {
  std::string str;
  for (int i = 0; i < indent_level; i++) {
    str += "  ";  // indent
  }
  str += label + ":\n";

  indent_level++;
  for (auto& inst : instructions) {
    for (int i = 0; i < indent_level; i++) {
      str += "  ";  // indent
    }

    str += std::visit([](auto& val) -> std::string { return val.toString(); },
                      inst) +
           "\n";
  }
  indent_level--;
  return str;
}

std::string NumberInst::toString() {
  return lv_name + " = " + std::to_string(val);
}
std::string AllocaInst::toString() {
  return "alloca: " + lv_name + " (" + types::toString(type) + ")";
}
std::string RefInst::toString() { return lv_name + " = ref " + val; }

std::string AssignInst::toString() { return lv_name + " =(overwrite) " + val; }

// std::string TimeInst::toString() { return lv_name + " = " + val + +"@" + time; }

std::string OpInst::toString() { return lv_name + " = " + lhs + op + rhs; }
FunInst::FunInst(const std::string& name, std::deque<std::string> newargs,
                 types::Value type, bool isrecursive)
    : MIRinstruction(name, std::move(type)),
      args(std::move(newargs)),
      isrecursive(isrecursive),
      hasself(false),
      body(std::make_shared<MIRblock>(name)) {}
std::string FunInst::toString() {
  std::string s;
  s += lv_name + " = fun";
  if (isrecursive) {
    s += "[rec]";
  }
  s += " " + join(args, " , ");
  if (!freevariables.empty()) {
    s += " fv{";
    for (auto& cap : freevariables) {
      s += cap + ",";
    }
    s = s.substr(0, s.size() - 1) + "}";
  }
  s += "\n";
  body->indent_level += 1;
  s += body->toString();
  body->indent_level -= 1;

  return s;
}

std::string MakeClosureInst::toString() {
  std::string s;
  s += lv_name + " = makeclosure " + fname + " ";
  for (auto& cap : captures) {
    s += cap + ",";
  }
  // s += body->toString();
  return s.substr(0, s.size() - 1);
}
std::string FcallInst::toString() {
  std::string s;
  auto timestr = (time)?"@"+time.value():"";
  return lv_name + " = app" + fcalltype_str[ftype] + " " + fname + " " +
         join(args, " , ") + timestr;
}

std::string ArrayInst::toString() {
  return lv_name + " = array " + name + " " + join(args, " , ");
}

std::string ArrayAccessInst::toString() {
  return lv_name + " = arrayaccess " + name + " " + index;
}

std::string IfInst::toString() {
  std::string s;
  s += lv_name + " = if " + cond + "\n";
  s += thenblock->toString();
  s += elseblock->toString();
  return s;
}
std::string ReturnInst::toString() { return "return " + val; }

}  // namespace mimium