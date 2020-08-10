/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/alphaconvert_visitor.hpp"
namespace mimium {

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