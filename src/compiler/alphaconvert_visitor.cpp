/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/alphaconvert_visitor.hpp"

#include <memory>

#include "basic/helper_functions.hpp"
namespace mimium {
AlphaConvertVisitor::AlphaConvertVisitor() : namecount(0), envcount(0) {
  init();
}
void AlphaConvertVisitor::init() {
  namecount = 0;
  envcount = 0;
  env.reset();
  env = std::make_shared<SymbolEnv>("root", nullptr);
}

AlphaConvertVisitor::~AlphaConvertVisitor() = default;
auto AlphaConvertVisitor::getResult() -> std::shared_ptr<ListAST> {
  return std::static_pointer_cast<ListAST>(res_stack.top());
};

auto AlphaConvertVisitor::createNewLVar(LvarAST& ast)
    -> std::unique_ptr<LvarAST> {
  std::string newname;
  if (env->isVariableSet(ast.getVal())) {
    newname = env->findVariable(ast.getVal());
  } else {
    newname = ast.getVal();
    if (!env->isRoot()) {  // do not rename variables in global scope
      newname += std::to_string(namecount++);
    }
    env->setVariableRaw(ast.getVal(), newname);  // register to map
  }
  return std::make_unique<LvarAST>(newname, ast.type);
}

void AlphaConvertVisitor::visit(LvarAST& ast) {
  auto newast = createNewLVar(ast);
  res_stack.push(std::move(newast));
}

void AlphaConvertVisitor::visit(RvarAST& ast) {
  std::string newname;

  if (env->isVariableSet(ast.getVal())) {
    newname = env->findVariable(ast.getVal());
  } else {
    newname = ast.getVal();
    Logger::debug_log("symbol " + ast.getVal() +
                          " not found, assumed to be external/builtin function",
                      Logger::DEBUG);
  }
  AST_Ptr newast = std::make_shared<RvarAST>(newname);
  res_stack.push(std::move(newast));
}
void AlphaConvertVisitor::visit(SelfAST& ast) {
  res_stack.push(std::make_shared<SelfAST>());
}

void AlphaConvertVisitor::visit(OpAST& ast) {
  ast.rhs->accept(*this);
  ast.lhs->accept(*this);
  auto newast = std::make_unique<OpAST>(ast.op, stackPopPtr(), stackPopPtr());
  res_stack.push(std::move(newast));
}
void AlphaConvertVisitor::visit(ListAST& ast) { listastvisit(ast); }
void AlphaConvertVisitor::visit(NumberAST& ast) { defaultvisit(ast); }
void AlphaConvertVisitor::visit(AssignAST& ast) {
  ast.getName()->accept(*this);
  auto newname = std::static_pointer_cast<LvarAST>(stackPopPtr());
  ast.getBody()->accept(*this);
  auto newast = std::make_unique<AssignAST>(std::move(newname), stackPopPtr());
  res_stack.push(std::move(newast));
}

void AlphaConvertVisitor::visit(ArgumentsAST& ast) { listastvisit(ast); }
void AlphaConvertVisitor::visit(FcallArgsAST& ast) { listastvisit(ast); }

void AlphaConvertVisitor::visit(ArrayAST& ast) { listastvisit(ast); }
void AlphaConvertVisitor::visit(ArrayAccessAST& ast) {
  ast.getName()->accept(*this);
  auto newname = std::static_pointer_cast<RvarAST>(stackPopPtr());
  ast.getIndex()->accept(*this);
  auto newindex = stackPopPtr();
  auto newast =
      std::make_unique<ArrayAccessAST>(std::move(newname), std::move(newindex));
  res_stack.push(std::move(newast));
}
void AlphaConvertVisitor::visit(FcallAST& ast) {
  ast.getFname()->accept(*this);
  auto newname = stackPopPtr();
  ast.getArgs()->accept(*this);
  auto newargs = std::static_pointer_cast<FcallArgsAST>(stackPopPtr());
  std::shared_ptr<AST>newtime=nullptr;
  if(ast.time!=nullptr){
  ast.time->accept(*this);
  newtime = stackPopPtr();
  }
  auto newast = std::make_unique<FcallAST>(
      std::move(newname), std::move(newargs), std::move(newtime));
  res_stack.push(std::move(newast));
}
void AlphaConvertVisitor::visit(LambdaAST& ast) {
  env = env->createNewChild("lambda" + std::to_string(envcount));
  envcount++;
  ast.getArgs()->accept(*this);  // register argument as unique name
  auto newargs = std::static_pointer_cast<ArgumentsAST>(stackPopPtr());
  ast.getBody()->accept(*this);
  auto newbody = stackPopPtr();
  auto newast = std::make_unique<LambdaAST>(std::move(newargs),
                                            std::move(newbody), ast.type);
  newast->isrecursive = ast.isrecursive;
  res_stack.push(std::move(newast));
  env = env->getParent();
}
void AlphaConvertVisitor::visit(IfAST& ast) {
  ast.getCond()->accept(*this);
  auto newcond = stackPopPtr();
  ast.getThen()->accept(*this);
  auto newthen = stackPopPtr();
  ast.getElse()->accept(*this);
  auto newelse = stackPopPtr();
  auto newast = std::make_unique<IfAST>(std::move(newcond), std::move(newthen),
                                        std::move(newelse), ast.isexpr);
  res_stack.push(std::move(newast));
};

void AlphaConvertVisitor::visit(ReturnAST& ast) {
  ast.getExpr()->accept(*this);
  res_stack.push(std::make_unique<ReturnAST>(stackPopPtr()));
}
void AlphaConvertVisitor::visit(ForAST& ast) {
  env = env->createNewChild("forloop" + std::to_string(envcount));
  envcount++;
  ast.getVar()->accept(*this);
  auto newvar = stackPopPtr();
  ast.getIterator()->accept(*this);
  auto newiter = stackPopPtr();
  ast.getExpression()->accept(*this);
  auto newexpr = stackPopPtr();
  auto newast = std::make_unique<IfAST>(std::move(newvar), std::move(newiter),
                                        std::move(newexpr));
  res_stack.push(std::move(newast));
  env = env->getParent();
}
void AlphaConvertVisitor::visit(DeclarationAST& ast) {
  // will not be called
}
// void AlphaConvertVisitor::visit(TimeAST& ast) {
//   ast.getExpr()->accept(*this);
//   auto newexpr = stackPopPtr();
//   ast.getTime()->accept(*this);
//   auto newtime = stackPopPtr();
//   auto newast =
//       std::make_unique<TimeAST>(std::move(newexpr), std::move(newtime));
//   res_stack.push(std::move(newast));
// }

void AlphaConvertVisitor::visit(StructAST& ast) {
  auto newast = std::make_unique<StructAST>();  // make empty
  for (auto& [key, val] : ast.map) {
    val->accept(*this);
    key->accept(*this);
    newast->addPair(stackPopPtr(), stackPopPtr());
  }
  res_stack.push(std::move(newast));
}
void AlphaConvertVisitor::visit(StructAccessAST& ast) {
  ast.getVal()->accept(*this);
  ast.getKey()->accept(*this);
  auto newast = std::make_unique<StructAccessAST>(stackPopPtr(), stackPopPtr());
  res_stack.push(std::move(newast));
}

}  // namespace mimium