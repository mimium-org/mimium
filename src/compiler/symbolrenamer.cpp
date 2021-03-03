/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/symbolrenamer.hpp"
namespace mimium {

// new alphaconverter
SymbolRenamer::SymbolRenamer() : SymbolRenamer(std::make_shared<RenameEnvironment>()) {}
SymbolRenamer::SymbolRenamer(std::shared_ptr<RenameEnvironment> env) : env(std::move(env)) {
  this->env->rename_map.emplace("dsp", "dsp");
}

AstPtr SymbolRenamer::rename(ast::Statements& ast) {
  auto newast = std::make_shared<ast::Statements>();
  for (const auto& statement : ast) {
    newast->push_back(std::visit(statement_renamevisitor, *statement));
  }
  return newast;
}
std::string SymbolRenamer::generateNewName(std::string const& name) {
  return name + std::to_string(namecount++);
}

std::string SymbolRenamer::getNewName(std::string const& name) {
  auto res = env->search(std::optional(name));
  return res.value_or(generateNewName(name));
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
  auto lhs = ast.lhs ? std::optional(rename(ast.lhs.value())) : std::nullopt;
  return ast::makeExpr(ast::Op{{{ast.debuginfo}}, ast.op, lhs, rename(ast.rhs)});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::Number& ast) { return ast::makeExpr(ast); }
ast::ExprPtr ExprRenameVisitor::operator()(ast::String& ast) {
  return ast::makeExpr(ast::String{{{ast.debuginfo}, ast.value}});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::Symbol& ast) {
  return ast::makeExpr(ast::Symbol{{{ast.debuginfo}}, renamer.searchFromEnv(ast.value)});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::Self& ast) { return ast::makeExpr(ast); }
ast::Block SymbolRenamer::renameBlock(ast::Block& ast) {
  env = env->expand();
  auto newstmts = rename(ast.stmts);
  auto newexpr = ast.expr.has_value() ? std::optional(renameExpr(ast.expr.value())) : std::nullopt;
  env = env->parent_env;
  return ast::Block{{{ast.debuginfo}}, *std::move(newstmts), std::move(newexpr)};
}

ast::ExprPtr ExprRenameVisitor::operator()(ast::Block& ast) {
  return ast::makeExpr(renamer.renameBlock(ast));
}
ast::LambdaArgs ExprRenameVisitor::renameLambdaArgs(ast::LambdaArgs& ast) {
  auto newargs = ast::transformArgs(
      ast.args, [&](ast::DeclVar a) { return renamer.lvar_renamevisitor.renameLambdaArgVar(a); });
  return ast::LambdaArgs{{{ast.debuginfo}}, std::move(newargs)};
}
ast::Lambda ExprRenameVisitor::renameLambda(ast::Lambda& ast) {
  renamer.env = renamer.env->expand();
  auto newargsast = renameLambdaArgs(ast.args);
  auto newbody = renamer.renameBlock(ast.body);
  renamer.env = renamer.env->parent_env;
  return ast::Lambda{{{ast.debuginfo}}, std::move(newargsast), std::move(newbody), ast.ret_type};
}

ast::ExprPtr ExprRenameVisitor::operator()(ast::Lambda& ast) {
  return ast::makeExpr(renameLambda(ast));
}

ast::FcallArgs SymbolRenamer::renameFcallArgs(ast::FcallArgs& ast) {
  auto newargs = ast::transformArgs(ast.args, [&](ast::ExprPtr e) { return renameExpr(e); });
  return ast::FcallArgs{{{ast.debuginfo}}, std::move(newargs)};
}

ast::Fcall SymbolRenamer::renameFcall(ast::Fcall& ast) {
  auto newargs = renameFcallArgs(ast.args);
  auto newfn = renameExpr(ast.fn);
  return ast::Fcall{{{ast.debuginfo}}, std::move(newfn), std::move(newargs)};
}
ast::If SymbolRenamer::renameIf(ast::If& ast) {
  auto newcond = renameExpr(ast.cond);
  auto newthen = renameExpr(ast.then_stmts);
  auto newelse =
      ast.else_stmts.has_value() ? std::optional(renameExpr(ast.else_stmts.value())) : std::nullopt;
  return ast::If{{{ast.debuginfo}}, newcond, newthen, newelse};
}

ast::ExprPtr ExprRenameVisitor::operator()(ast::Fcall& ast) {
  return ast::makeExpr(renamer.renameFcall(ast));
}

ast::ExprPtr ExprRenameVisitor::operator()(ast::Struct& ast) {
  auto newargs =
      ast::transformArgs(ast.args, [&](ast::ExprPtr e) { return renamer.renameExpr(e); });
  return ast::makeExpr(ast::Struct{{{ast.debuginfo}}, std::move(newargs)});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::StructAccess& ast) {
  return ast::makeExpr(ast::StructAccess{{{ast.debuginfo}}, rename(ast.stru), ast.field});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::ArrayInit& ast) {
  auto newargs =
      ast::transformArgs(ast.args, [&](ast::ExprPtr e) { return renamer.renameExpr(e); });
  return ast::makeExpr(ast::ArrayInit{{{ast.debuginfo}}, std::move(newargs)});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::ArrayAccess& ast) {
  return ast::makeExpr(ast::ArrayAccess{{{ast.debuginfo}}, rename(ast.array), rename(ast.index)});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::Tuple& ast) {
  auto newargs =
      ast::transformArgs(ast.args, [&](ast::ExprPtr e) { return renamer.renameExpr(e); });
  return ast::makeExpr(ast::Tuple{{{ast.debuginfo}}, std::move(newargs)});
}

ast::ExprPtr ExprRenameVisitor::operator()(ast::If& ast) {
  return ast::makeExpr(renamer.renameIf(ast));
}

using LvarRenameVisitor = SymbolRenamer::LvarRenameVisitor;
ast::DeclVar LvarRenameVisitor::renameDeclVar(ast::DeclVar& ast) {
  auto newname = renamer.getNewName(ast.value.value);
  renamer.env->addToMap(ast.value.value, newname);
  return ast::DeclVar{{{ast.debuginfo}}, ast::Symbol{{ast.value.debuginfo}, newname}, ast.type};
}
ast::DeclVar LvarRenameVisitor::renameLambdaArgVar(ast::DeclVar& ast) {
  auto newname = renamer.generateNewName(ast.value.value);
  renamer.env->addToMap(ast.value.value, newname);
  return ast::DeclVar{{{ast.debuginfo}}, ast::Symbol{{ast.value.debuginfo}, newname}, ast.type};
}

ast::Lvar LvarRenameVisitor::operator()(ast::DeclVar& ast) { return renameDeclVar(ast); }

ast::Lvar LvarRenameVisitor::operator()(ast::ArrayLvar& ast) {
  return ast::ArrayLvar{
      {{ast.debuginfo}}, renamer.renameExpr(ast.array), renamer.renameExpr(ast.index)};
}
ast::Lvar LvarRenameVisitor::operator()(ast::TupleLvar& ast) {
  auto newargs = ast::transformArgs(
      ast.args, [&](ast::DeclVar a) { return renamer.lvar_renamevisitor.renameDeclVar(a); });
  return ast::TupleLvar{{{ast.debuginfo}}, newargs};
}
using StatementRenameVisitor = SymbolRenamer::StatementRenameVisitor;

StatementPtr StatementRenameVisitor::operator()(ast::Fdef& ast) {
  auto newlvar = renamer.lvar_renamevisitor.renameDeclVar(ast.lvar);
  auto newfun = renamer.expr_renamevisitor.renameLambda(ast.fun);
  return ast::makeStatement(ast::Fdef{{{ast.debuginfo}}, std::move(newlvar), std::move(newfun)});
}

StatementPtr StatementRenameVisitor::operator()(ast::Assign& ast) {
  auto newlvar = renamer.renameLvar(ast.lvar);
  auto newrvar = renamer.renameExpr(ast.expr);
  return ast::makeStatement(ast::Assign{{{ast.debuginfo}}, std::move(newlvar), std::move(newrvar)});
}
StatementPtr StatementRenameVisitor::operator()(ast::TypeAssign& ast) {
    return ast::makeStatement(ast);
}
StatementPtr StatementRenameVisitor::operator()(ast::Return& ast) {
  return ast::makeStatement(ast::Return{{{ast.debuginfo}}, renamer.renameExpr(ast.value)});
}
// StatementPtr StatementRenameVisitor::operator()(ast::Declaration& ast) {}
StatementPtr StatementRenameVisitor::operator()(ast::Time& ast) {
  return ast::makeStatement(
      ast::Time{{{ast.debuginfo}}, renamer.renameFcall(ast.fcall), renamer.renameExpr(ast.when)});
}
StatementPtr StatementRenameVisitor::operator()(ast::Fcall& ast) {
  return ast::makeStatement(renamer.renameFcall(ast));
}
StatementPtr StatementRenameVisitor::operator()(ast::If& ast) {
  return ast::makeStatement(renamer.renameIf(ast));
}

StatementPtr StatementRenameVisitor::operator()(ast::For& ast) {
  auto newiter = renamer.renameExpr(ast.iterator);
  renamer.env = renamer.env->expand();
  auto newindex = renamer.lvar_renamevisitor.renameDeclVar(ast.index);
  auto newstmts = renamer.renameBlock(ast.statements);
  renamer.env = renamer.env->parent_env;
  return ast::makeStatement(
      ast::For{{{ast.debuginfo}}, std::move(newindex), std::move(newiter), std::move(newstmts)});
}

}  // namespace mimium