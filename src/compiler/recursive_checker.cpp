/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
 
#include "compiler/recursive_checker.hpp"


namespace mimium {
RecursiveChecker::RecursiveChecker() {}

void RecursiveChecker::init() {}

void RecursiveChecker::visit(AssignAST& ast) {
  auto body = ast.getBody();
  if (body->getid() == LAMBDA) {
    tmp_fname = ast.getName()->getVal();
  }
  body->accept(*this);
  tmp_fname = "";
}

void RecursiveChecker::visit(LvarAST& ast) {
  // do nothing
}

void RecursiveChecker::visit(RvarAST& ast) {
  // do nothing
}

void RecursiveChecker::visit(SelfAST& ast) {
  hasself = true;
}
void RecursiveChecker::visit(OpAST& ast) {
  ast.lhs->accept(*this);
  ast.rhs->accept(*this);
}
void RecursiveChecker::visit(ListAST& ast) {
  for (auto& line : ast.getElements()) {
    line->accept(*this);
  }
}
void RecursiveChecker::visit(NumberAST& /*ast*/) {  // do nothing
}

void RecursiveChecker::visit(StringAST& /*ast*/) {  // do nothing
}


void RecursiveChecker::visit(ArgumentsAST& ast) {
  // do nothing
}
void RecursiveChecker::visit(FcallArgsAST& ast) {
  // do nothing(if overloading or patter macher is implemented, need to do
  // something)
}
void RecursiveChecker::visit(ArrayAST& ast) {
  // do nothing
}
void RecursiveChecker::visit(ArrayAccessAST& ast) {
  // do nothing
}

void RecursiveChecker::visit(FcallAST& ast) {
  if(ast.getFname()->getid() == RVAR){
    // todo: how to check anonymous recursive lambda?(Z combinator)
  auto fname = std::static_pointer_cast<RvarAST>(ast.getFname())->getVal();
  if (target_fname == fname) {
    isrecursive = true;
  }
  }
  for(auto& a:ast.getArgs()->getElements()){
    a->accept(*this);
  }
  if(ast.time!=nullptr){
    ast.time->accept(*this);
  }
}
void RecursiveChecker::visit(LambdaAST& ast) {
  isrecursive = false;  
  hasself = false;   // reset flag
  if (!tmp_fname.empty()) {  // ignore anonymous function
    target_fname = tmp_fname;
    ast.getBody()->accept(*this);
    ast.isrecursive = isrecursive;
    ast.hasself = hasself;
  }
}
void RecursiveChecker::visit(IfAST& ast) {
  ast.getCond()->accept(*this);

  ast.getThen()->accept(*this);
  ast.getElse()->accept(*this);
};

void RecursiveChecker::visit(ReturnAST& ast) {
  ast.getExpr()->accept(*this);
}
void RecursiveChecker::visit(ForAST& ast) {
 //TODO(tomoya)
}
void RecursiveChecker::visit(DeclarationAST& ast) {
  // will not be called
}
// void RecursiveChecker::visit(TimeAST& ast) {
//   ast.getExpr()->accept(*this);
//   ast.getTime()->accept(*this);

// }

void RecursiveChecker::visit(StructAST& ast) {}
void RecursiveChecker::visit(StructAccessAST& ast) {}

}  // namespace mimium
