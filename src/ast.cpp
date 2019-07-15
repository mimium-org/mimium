
#include "ast.hpp"

std::ostream& NumberAST::to_string(std::ostream& ss) {
  // type matching
  ss << val;

  return ss;
}

std::ostream& SymbolAST::to_string(std::ostream& ss) {
  ss << val;
  return ss;
}

OpAST::OpAST(std::string Op, AST_Ptr LHS, AST_Ptr RHS)
    : op(Op), lhs(std::move(LHS)), rhs(std::move(RHS)) {
  id = OP;
  op_id = optable[Op];
}

std::ostream& OpAST::to_string(std::ostream& ss) {
  ss << "(" << op << " ";
  lhs->to_string(ss);
  ss << " ";
  rhs->to_string(ss);
  ss << ")";

  return ss;
}

std::ostream& ListAST::to_string(std::ostream& ss) {
  int size = asts.size();
  if (size == 1) {
    asts[0]->to_string(ss);
  } else {
    int count = 1;
    ss << "(";
    for (auto& elem : asts) {
      elem->to_string(ss);
      if (size != count) {
        ss << " ";
      }
      count++;
    }
    ss << ")";
  }
  return ss;
}

std::ostream& AbstractListAST::to_string(std::ostream& ss) {
  ss << "(";
  int count = 1;
  for (auto& elem : elements) {
    elem->to_string(ss);
    if(count != elements.size()) ss << " ";
    count++;
  }
  ss << ")";
  return ss;
}
std::ostream& ArrayAccessAST::to_string(std::ostream& ss) {
  ss << "arrayaccess ";
  name->to_string(ss);
  ss << " ";
  index->to_string(ss);
  return ss;
}

std::ostream& ReturnAST::to_string(std::ostream& ss) {
  ss << "(return ";
  expr->to_string(ss);
  ss << ")";

  return ss;
}

std::ostream& LambdaAST::to_string(std::ostream& ss) {
  ss << "(lambda ";
  args->to_string(ss);
  ss << " ";
  body->to_string(ss);
  ss << ")";
  return ss;
}

std::ostream& AssignAST::to_string(std::ostream& ss) {
  ss << "("
     << "assign"
     << " ";
  symbol->to_string(ss);
  ss << " ";
  expr->to_string(ss);
  ss << ")";
  return ss;
}

std::ostream& FcallAST::to_string(std::ostream& ss) {
  ss << "(";
  fname->to_string(ss);
  ss << " ";
  args->to_string(ss);
  ss << ")";
  return ss;
}
std::ostream& DeclarationAST::to_string(std::ostream& ss) {
  ss << "(";
  fname->to_string(ss);
  ss << " ";
  args->to_string(ss);
  ss << ")";
  return ss;
}
std::ostream& IfAST::to_string(std::ostream& ss) {
  ss << "if ";
  condition->to_string(ss);
  ss << " ";
  thenstatement->to_string(ss);
  ss << " ";
  elsestatement->to_string(ss);
  return ss;
}
std::ostream& ForAST::to_string(std::ostream& ss) {
  ss << "(for ";
  var->to_string(ss);
  ss << " ";
  iterator->to_string(ss);
  ss << " ";
  expression->to_string(ss);
  ss << ")";
  return ss;
}

std::ostream& TimeAST::to_string(std::ostream& ss) {
  expr->to_string(ss);
  ss << "@";
  time->to_string(ss);
  return ss;
}