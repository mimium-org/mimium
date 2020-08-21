/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/alphaconvert_visitor.hpp"
namespace mimium {

// new alphaconverter
SymbolRenamer::SymbolRenamer()
    : SymbolRenamer(std::make_shared<RenameEnvironment>()) {}
SymbolRenamer::SymbolRenamer(std::shared_ptr<RenameEnvironment> env)
    : env(std::move(env)) {
  this->env->rename_map.emplace("dsp", "dsp");
}

AstPtr SymbolRenamer::rename(ast::Statements& ast) {
  auto newast = std::make_shared<ast::Statements>();
  for (const auto& statement : ast) {
    newast->push_back(std::visit(statement_renamevisitor, *statement));
  }
  return std::move(newast);
}

std::string SymbolRenamer::getNewName(std::string const& name) {
  auto res = env->search(std::optional(name));
  if(res == std::nullopt){
    return name + std::to_string(namecount++);
  }
  return res.value();
}
std::string SymbolRenamer::searchFromEnv(std::string const& name) {
  auto res = env->search(std::optional(name));
  if (res == std::nullopt) {
    // the variable not found, assumed to be external symbol at this stage.
    return name;
  }
  return res.value();
}

using ExprRenameVisitor = SymbolRenamer::ExprRenameVisitor;

ast::ExprPtr ExprRenameVisitor::operator()(ast::Op& ast) {
  auto lhs = (ast.lhs) ? std::optional(std::visit(*this, *ast.lhs.value()))
                       : std::nullopt;
  return ast::makeExpr(
      ast::Op{ast.debuginfo, ast.op, lhs, std::visit(*this, *ast.rhs)});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::Number& ast) {
  return ast::makeExpr(ast);
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::String& ast) {
  return ast::makeExpr(
      ast::String{ast.debuginfo,ast.value});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::Rvar& ast) {
  return ast::makeExpr(
      ast::Rvar{ast.debuginfo, renamer.searchFromEnv(ast.value)});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::Self& ast) {
  return ast::makeExpr(ast);
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::Lambda& ast) {
  renamer.env = renamer.env->expand();
  std::deque<ast::Lvar> newargs;
  for (auto&& a : ast.args.args) {
    auto newname = renamer.getNewName(a.value);
    renamer.env->addToMap(a.value, newname);
    newargs.emplace_back(ast::Lvar{a.debuginfo, newname, a.type});
  }
  auto newargsast = ast::LambdaArgs{ast.args.debuginfo, std::move(newargs)};
  auto newbody = *renamer.rename(ast.body);
  renamer.env = renamer.env->parent_env;
  return ast::makeExpr(ast::Lambda{ast.debuginfo, std::move(newargsast),
                                         std::move(newbody), ast.ret_type});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::Fcall& ast) {
  std::deque<ast::ExprPtr> newargs;
  for (auto&& a : ast.args.args) {
    newargs.emplace_back(std::visit(*this, *a));
  }
  auto newargsast = ast::FcallArgs{ast.args.debuginfo, std::move(newargs)};
  auto newfn = std::visit(*this, *ast.fn);
  auto newfcall =
      ast::Fcall{ast.debuginfo, std::move(newfn), std::move(newargsast)};
  return ast::makeExpr(std::move(newfcall));
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::Time& ast) {
  auto newfcall = rv::get<ast::Fcall>(*(*this)(ast.fcall));
  auto newwhen = std::visit(*this, *ast.when);
  return ast::makeExpr(
      ast::Time{ast.debuginfo, std::move(newfcall), std::move(newwhen)});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::Struct& ast) {
  std::deque<ast::ExprPtr> newargs;
  for (auto&& a : ast.args) {
    newargs.emplace_back(std::visit(*this, *a));
  }
  return ast::makeExpr(ast::Struct{ast.debuginfo, std::move(newargs)});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::StructAccess& ast) {
  auto newfield = std::visit(*this, *ast.field);
  auto newstr = std::visit(*this, *ast.stru);
  return ast::makeExpr(ast::StructAccess{
      ast.debuginfo, std::move(newfield), std::move(newstr)});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::ArrayInit& ast) {
  std::deque<ast::ExprPtr> newargs;
  for (auto&& a : ast.args) {
    newargs.emplace_back(std::visit(*this, *a));
  }
  return ast::makeExpr(ast::ArrayInit{ast.debuginfo, std::move(newargs)});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::ArrayAccess& ast) {
  auto newfield = std::visit(*this, *ast.array);
  auto newstr = std::visit(*this, *ast.index);
  return ast::makeExpr(ast::ArrayAccess{
      ast.debuginfo, std::move(newfield), std::move(newstr)});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::Tuple& ast) {
  std::deque<ast::ExprPtr> newargs;
  for (auto&& a : ast.args) {
    newargs.emplace_back(std::visit(*this, *a));
  }
  return ast::makeExpr(ast::ArrayInit{ast.debuginfo, std::move(newargs)});
}

using StatementRenameVisitor = SymbolRenamer::StatementRenameVisitor;
StatementPtr StatementRenameVisitor::operator()(ast::Assign& ast) {
  auto newname = renamer.getNewName(ast.lvar.value);
  renamer.env->addToMap(ast.lvar.value, newname);
  auto newrvar = std::visit(renamer.expr_renamevisitor, *ast.expr);
  auto newlvar = ast::Lvar{ast.lvar.debuginfo, newname, ast.lvar.type};
  return ast::makeStatement(
      ast::Assign{ast.debuginfo, std::move(newlvar), std::move(newrvar)});
}
StatementPtr StatementRenameVisitor::operator()(ast::Return& ast) {
  auto newrvar = std::visit(renamer.expr_renamevisitor, *ast.value);
  return ast::makeStatement(
      ast::Return{ast.debuginfo, std::move(newrvar)});
}
// StatementPtr StatementRenameVisitor::operator()(ast::Declaration& ast) {}

StatementPtr StatementRenameVisitor::operator()(ast::For& ast) {
  auto newiter = std::visit(renamer.expr_renamevisitor, *ast.iterator);
  renamer.env = renamer.env->expand();
  auto newname = renamer.getNewName(ast.index.value);
  renamer.env->addToMap(ast.index.value, newname);
  auto newindex = ast::Lvar{ast.index.debuginfo, newname, ast.index.type};
  auto newstmts = renamer.rename(ast.statements);
  renamer.env = renamer.env->parent_env;
  return ast::makeStatement(ast::For{ast.debuginfo, std::move(newindex),
                                           std::move(newiter),
                                           std::move(*newstmts)});
}
StatementPtr StatementRenameVisitor::operator()(ast::If& ast) {
  auto newcond = std::visit(renamer.expr_renamevisitor, *ast.cond);
  auto newthenptr = renamer.rename(ast.then_stmts);
  auto newelse = ast.else_stmts;
  if (ast.else_stmts.has_value()) {
    newelse = std::optional(*renamer.rename(ast.else_stmts.value()));
  }
  return ast::makeStatement(
      ast::If{ast.debuginfo, newcond, *newthenptr, newelse});
}
StatementPtr StatementRenameVisitor::operator()(ast::ExprPtr& ast) {
  return ast::makeStatement(std::visit(renamer.expr_renamevisitor, *ast));
}

}  // namespace mimium