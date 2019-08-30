
#include "ast.hpp"


std::string NumberAST::toString() {
  std::stringstream ss;
  ss<<val;
  return ss.str();
}


std::string SymbolAST::toString() {
  return val;
}

OpAST::OpAST(std::string Op, AST_Ptr LHS, AST_Ptr RHS)
    : op(Op), lhs(std::move(LHS)), rhs(std::move(RHS)) {
  id = OP;
  op_id = optable[Op];
}

std::string OpAST::toString() {
  return "(" + op + " " + lhs->toString() + " " + rhs->toString() + ")";
}


std::string ListAST::toString() {
  std::stringstream ss;
    int size = asts.size();
  if (size == 1) {
    ss  << asts[0]->toString();
  } else {
    int count = 1;
    ss << "(";
    for (auto& elem : asts) {
      ss << elem->toString();
      if (size != count) ss << " ";
      count++;
    }
    ss << ")";
  }
  return ss.str();
}

std::string AbstractListAST::toString() {
  std::stringstream ss;
  ss << "(";
  int count = 1;
  for (auto& elem : elements) {
    ss << elem->toString();
    if(count != elements.size()) ss << " ";
    count++;
  }
  ss << ")";
  return ss.str();
}


std::string ArrayAccessAST::toString() {
  return "arrayaccess " + name->toString() + " " + index->toString();
}


std::string ReturnAST::toString() {
  return "(return " + expr->toString() + ")";
}


std::string LambdaAST::toString() {
  return "(lambda " + args->toString() + " " + body->toString() + ")";
}

std::string AssignAST::toString() {
  return "(assign " + symbol->toString() + " " + expr->toString() + ")";
}

std::string FcallAST::toString() {
  return "(" + fname->toString() + " " + args->toString() + ")";
}


std::string DeclarationAST::toString() {
  return "(" + fname->toString() + " " + args->toString() + ")";
}

std::string IfAST::toString() {
  return "if " + condition->toString() + " " + thenstatement->toString() + " " + elsestatement->toString();
}


std::string ForAST::toString() {
  return "(for " + var->toString() + " " + iterator->toString() + " " + expression->toString() + ")";
}

std::string TimeAST::toString() {
  return expr->toString() + "@" + time->toString();
}