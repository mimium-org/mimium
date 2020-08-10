/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/alphaconvert_visitor.hpp"

#include <memory>
#include <utility>

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
void AlphaConvertVisitor::visit(StringAST& ast) { defaultvisit(ast); }

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
  std::shared_ptr<AST> newtime = nullptr;
  if (ast.time != nullptr) {
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

// new alphaconverter
SymbolRenamer::SymbolRenamer(std::shared_ptr<RenameEnvironment> env)
    : env(std::move(env)) {}

AstPtr SymbolRenamer::rename(newast::Statements& ast) {
  auto newast = std::make_shared<newast::Statements>();
  for (const auto& statement : ast) {
    newast->push_back(std::visit(statement_renamevisitor, *statement));
  }
  return std::move(newast);
}

std::string SymbolRenamer::getNewName(std::string const& name) {
  return name + std::to_string(namecount++);
}
std::string SymbolRenamer::searchFromEnv(std::string const& name) {
  auto res = env->search(std::optional(name));
  if (res == std::nullopt) {
    throw std::runtime_error("variable " + name + " not found.");
    return name;
  }
  return res.value();
}

using ExprRenameVisitor = SymbolRenamer::ExprRenameVisitor;

newast::ExprPtr ExprRenameVisitor::operator()(newast::Op& ast) {
  auto lhs = (ast.lhs) ? std::optional(std::visit(*this, *ast.lhs.value()))
                       : std::nullopt;
  return newast::makeExpr(
      newast::Op{ast.debuginfo, ast.op, lhs, std::visit(*this, *ast.rhs)});
}
newast::ExprPtr ExprRenameVisitor::operator()(newast::Number& ast) {
  return newast::makeExpr(ast);
}
newast::ExprPtr ExprRenameVisitor::operator()(newast::String& ast) {
  return newast::makeExpr(
      newast::String{ast.debuginfo, renamer.searchFromEnv(ast.value)});
}
newast::ExprPtr ExprRenameVisitor::operator()(newast::Rvar& ast) {
  return newast::makeExpr(
      newast::Rvar{ast.debuginfo, renamer.searchFromEnv(ast.value)});
}
newast::ExprPtr ExprRenameVisitor::operator()(newast::Self& ast) {
  return newast::makeExpr(ast);
}
newast::ExprPtr ExprRenameVisitor::operator()(newast::Lambda& ast) {
  renamer.env = renamer.env->expand();
  std::deque<newast::Lvar> newargs;
  for (auto&& a : ast.args.args) {
    auto newname = renamer.getNewName(a.value);
    renamer.env->addToMap(a.value, newname);
    newargs.emplace_back(newast::Lvar{a.debuginfo, newname});
  }
  auto&& newargsast =
      newast::LambdaArgs{ast.args.debuginfo, std::move(newargs)};
  auto&& newbody = *renamer.rename(ast.body);
  renamer.env = renamer.env->parent_env;
  return newast::makeExpr(
      newast::Lambda{ast.debuginfo, newargsast, newbody, ast.ret_type});
}
newast::ExprPtr ExprRenameVisitor::operator()(newast::Fcall& ast) {
  std::deque<newast::ExprPtr> newargs;
  for (auto&& a : ast.args.args) {
    newargs.emplace_back(std::visit(*this, *a));
  }
  auto&& newargsast = newast::FcallArgs{ast.args.debuginfo, newargs};
  auto&& newfn = std::visit(*this, *ast.fn);
  auto&& newfcall = newast::Fcall{ast.debuginfo, newfn, newargsast};
  return newast::makeExpr(newfcall);
}
newast::ExprPtr ExprRenameVisitor::operator()(newast::Time& ast) {
  auto&& newfcall = rv::get<newast::Fcall>(*(*this)(ast.fcall));
  auto&& newwhen = std::visit(*this, *ast.when);
  return newast::makeExpr(newast::Time{ast.debuginfo, newfcall, newwhen});
}
newast::ExprPtr ExprRenameVisitor::operator()(newast::Struct& ast) {
  std::deque<newast::ExprPtr> newargs;
  for (auto&& a : ast.args) {
    newargs.emplace_back(std::visit(*this, *a));
  }
  return newast::makeExpr(newast::Struct{ast.debuginfo, std::move(newargs)});
}
newast::ExprPtr ExprRenameVisitor::operator()(newast::StructAccess& ast) {
  auto&& newfield = std::visit(*this, *ast.field);
  auto&& newstr = std::visit(*this, *ast.stru);
  return newast::makeExpr(
      newast::StructAccess{ast.debuginfo, newfield, newstr});
}
newast::ExprPtr ExprRenameVisitor::operator()(newast::ArrayInit& ast) {
  std::deque<newast::ExprPtr> newargs;
  for (auto&& a : ast.args) {
    newargs.emplace_back(std::visit(*this, *a));
  }
  return newast::makeExpr(newast::ArrayInit{ast.debuginfo, std::move(newargs)});
}
newast::ExprPtr ExprRenameVisitor::operator()(newast::ArrayAccess& ast) {
  auto&& newfield = std::visit(*this, *ast.array);
  auto&& newstr = std::visit(*this, *ast.index);
  return newast::makeExpr(newast::ArrayAccess{ast.debuginfo, newfield, newstr});
}
newast::ExprPtr ExprRenameVisitor::operator()(newast::Tuple& ast) {
  std::deque<newast::ExprPtr> newargs;
  for (auto&& a : ast.args) {
    newargs.emplace_back(std::visit(*this, *a));
  }
  return newast::makeExpr(newast::ArrayInit{ast.debuginfo, std::move(newargs)});
}

using StatementRenameVisitor = SymbolRenamer::StatementRenameVisitor;
StatementPtr StatementRenameVisitor::operator()(newast::Assign& ast) {
  auto&& newrvar = std::visit(renamer.expr_renamevisitor, *ast.expr);
  auto newname = renamer.getNewName(ast.lvar.value);
  renamer.env->addToMap(ast.lvar.value, newname);
  auto&& newlvar = newast::Lvar{ast.lvar.debuginfo, newname, ast.lvar.type};
  return newast::makeStatement(newast::Assign{ast.debuginfo, newlvar, newrvar});
}
StatementPtr StatementRenameVisitor::operator()(newast::Return& ast) {
  auto&& newrvar = std::visit(renamer.expr_renamevisitor, *ast.value);
  return newast::makeStatement(newast::Return{ast.debuginfo, newrvar});
}
// StatementPtr StatementRenameVisitor::operator()(newast::Declaration& ast) {}

StatementPtr StatementRenameVisitor::operator()(newast::For& ast) {
  auto&& newiter = std::visit(renamer.expr_renamevisitor, *ast.iterator);
  renamer.env = renamer.env->expand();
  auto newname = renamer.getNewName(ast.index.value);
  renamer.env->addToMap(ast.index.value, newname);
  auto&& newindex = newast::Lvar{ast.index.debuginfo, newname, ast.index.type};
  auto newstmts = renamer.rename(ast.statements);
  renamer.env = renamer.env->parent_env;
  return newast::makeStatement(
      newast::For{ast.debuginfo, newindex, newiter, std::move(*newstmts)});
}
StatementPtr StatementRenameVisitor::operator()(newast::If& ast) {
  auto&& newcond = std::visit(renamer.expr_renamevisitor, *ast.cond);
  auto newthenptr = renamer.rename(ast.then_stmts);
  auto newelse = ast.else_stmts;
  if (ast.else_stmts.has_value()) {
    newelse = std::optional(*renamer.rename(ast.else_stmts.value()));
  }
  return newast::makeStatement(
      newast::If{ast.debuginfo, newcond, *newthenptr, newelse});
}
StatementPtr StatementRenameVisitor::operator()(newast::ExprPtr& ast) {
  return newast::makeStatement(std::visit(renamer.expr_renamevisitor, *ast));
}

}  // namespace mimium