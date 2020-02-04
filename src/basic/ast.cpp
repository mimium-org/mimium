/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "basic/ast.hpp"
namespace mimium {
std::string NumberAST::toString() {
  std::stringstream ss;
  ss << val;
  return ss.str();
}
std::string NumberAST::toJson() { return toString(); }
std::string StringAST::toString() { return val; }
std::string StringAST::toJson() { return val; }

std::string SymbolAST::toString() { return val; }
std::string SymbolAST::toJson() { return "'" + toString() + "'"; }

OpAST::OpAST(std::string Op, AST_Ptr LHS, AST_Ptr RHS)
    : op(Op), lhs(std::move(LHS)), rhs(std::move(RHS)) {
  id = OP;
  op_id = optable[Op];
}

std::string OpAST::toString() {
  return "(" + op + " " + lhs->toString() + " " + rhs->toString() + ")";
}
std::string OpAST::toJson() {
  return "[ '" + op + "', " + lhs->toJson() + ", " + rhs->toJson() + "]";
}

std::string ArrayAccessAST::toString() {
  return "arrayaccess " + name->toString() + " " + index->toString();
}
std::string ArrayAccessAST::toJson() {
  return "[ 'arrayaccess' ," + name->toJson() + ", " + index->toJson() + "]";
}

std::string ReturnAST::toString() {
  return "(return " + expr->toString() + ")";
}

std::string ReturnAST::toJson() {
  return "[ 'return' ," + expr->toJson() + "]";
}

std::string LambdaAST::toString() {
  return "(lambda " + args->toString() + " " + body->toString() + ")";
}

std::string LambdaAST::toJson() {
  return "[ 'lambda' ," + args->toJson() + ", " + body->toJson() + "]";
}

std::string AssignAST::toString() {
  return "(assign " + symbol->toString() + " " + expr->toString() + ")";
}

std::string AssignAST::toJson() {
  return "[ 'assign' ," + symbol->toJson() + ", " + expr->toJson() + "]";
}

std::string FcallAST::toString() {
  return "(" + fname->toString() + " " + args->toString() +
         ((time) ? "@" + time->toString() : "") + ")";
}
std::string FcallAST::toJson() {
  return "[" + fname->toJson() + ", " + args->toJson() +
         ((time) ? "@" + time->toString() : "") + "]";
}

std::string DeclarationAST::toString() {
  return "(" + fname->toString() + " " + args->toString() + ")";
}

std::string DeclarationAST::toJson() {
  return "[" + fname->toJson() + ", " + args->toJson() + "]";
}

std::string IfAST::toString() {
  return "(if " + condition->toString() + " " + thenstatement->toString() +
         " " + elsestatement->toString() + ")";
}
std::string IfAST::toJson() {
  return "['if' ," + condition->toJson() + ", " + thenstatement->toJson() +
         ", " + elsestatement->toJson() + "]";
}

std::string ForAST::toString() {
  return "(for " + var->toString() + " " + iterator->toString() + " " +
         expression->toString() + ")";
}
std::string ForAST::toJson() {
  return "['for' ," + var->toJson() + ", " + iterator->toJson() + ", " +
         expression->toJson() + "]";
}

// std::string TimeAST::toString() {
//   return expr->toString() + "@" + time->toString();
// }

// std::string TimeAST::toJson() {
//   return "[ 'time', " + expr->toJson() + ", " + time->toJson() + "]";
// }

std::string StructAST::toString() {  // this is not lisp like style,,
  std::stringstream ss;
  int count = 0;
  ss << "{";
  for (auto& [k, v] : map) {
    ss << k->toString() << " : " << v->toString();
    count++;
    if (map.size() < count) ss << ",\n";
  }
  ss << "}";
  return ss.str();
}
std::string StructAST::toJson() { return this->toString(); }
std::string StructAccessAST::toString() {
  return "structaccess " + key->toString() + " " + val->toString();
}
std::string StructAccessAST::toJson() {
  return "[structaccess , " + key->toString() + " , " + val->toString() + "]";
}
}  // namespace mimium
