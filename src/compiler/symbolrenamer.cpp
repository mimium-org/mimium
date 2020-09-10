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
  return std::move(newast);
}

std::string SymbolRenamer::getNewName(std::string const& name) {
  auto res = env->search(std::optional(name));
  if (res == std::nullopt) { return name + std::to_string(namecount++); }
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
  auto lhs = ast.lhs ? std::optional(rename(ast.lhs.value())) : std::nullopt;
  return ast::makeExpr(ast::Op{ast.debuginfo, ast.op, lhs, rename(ast.rhs)});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::Number& ast) { return ast::makeExpr(ast); }
ast::ExprPtr ExprRenameVisitor::operator()(ast::String& ast) {
  return ast::makeExpr(ast::String{ast.debuginfo, ast.value});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::Rvar& ast) {
  return ast::makeExpr(ast::Rvar{ast.debuginfo, renamer.searchFromEnv(ast.value)});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::Self& ast) { return ast::makeExpr(ast); }
ast::Block SymbolRenamer::renameBlock(ast::Block& ast) {
  env = env->expand();
  auto newstmts = rename(ast.stmts);
  auto newexpr = ast.expr.has_value() ? std::optional(renameExpr(ast.expr.value())) : std::nullopt;
  env = env->parent_env;
  return ast::Block{{ast.debuginfo}, *std::move(newstmts), std::move(newexpr)};
}

ast::ExprPtr ExprRenameVisitor::operator()(ast::Block& ast) {
  return ast::makeExpr(renamer.renameBlock(ast));
}
ast::LambdaArgs ExprRenameVisitor::renameLambdaArgs(ast::LambdaArgs& ast) {
  auto newargs = ast::transformArgs(ast.args, [&](ast::Lvar a) {
    auto newname = renamer.getNewName(a.value);
    renamer.env->addToMap(a.value, newname);
    return ast::Lvar{a.debuginfo, newname, a.type};
  });
  return ast::LambdaArgs{ast.debuginfo, std::move(newargs)};
}
ast::Lambda ExprRenameVisitor::renameLambda(ast::Lambda& ast) {
  renamer.env = renamer.env->expand();
  auto newargsast = renameLambdaArgs(ast.args);
  auto newbody = renamer.renameBlock(ast.body);
  renamer.env = renamer.env->parent_env;
  return ast::Lambda{ast.debuginfo, std::move(newargsast), std::move(newbody), ast.ret_type};
}

ast::ExprPtr ExprRenameVisitor::operator()(ast::Lambda& ast) {
  return ast::makeExpr(renameLambda(ast));
}

ast::FcallArgs SymbolRenamer::renameFcallArgs(ast::FcallArgs& ast) {
  auto newargs = ast::transformArgs(ast.args, [&](ast::ExprPtr e) { return renameExpr(e); });
  return ast::FcallArgs{ast.debuginfo, std::move(newargs)};
}

ast::Fcall SymbolRenamer::renameFcall(ast::Fcall& ast) {
  auto newargs = renameFcallArgs(ast.args);
  auto newfn = renameExpr(ast.fn);
  return ast::Fcall{ast.debuginfo, std::move(newfn), std::move(newargs)};
}
ast::If SymbolRenamer::renameIf(ast::If& ast) {
  auto newcond = renameExpr(ast.cond);
  auto newthen = renameExpr(ast.then_stmts);
  auto newelse =
      ast.else_stmts.has_value() ? std::optional(renameExpr(ast.else_stmts.value())) : std::nullopt;
  return ast::If{ast.debuginfo, newcond, newthen, newelse};
}

ast::ExprPtr ExprRenameVisitor::operator()(ast::Fcall& ast) {
  return ast::makeExpr(renamer.renameFcall(ast));
}

ast::ExprPtr ExprRenameVisitor::operator()(ast::Struct& ast) {
  auto newargs =
      ast::transformArgs(ast.args, [&](ast::ExprPtr e) { return renamer.renameExpr(e); });
  return ast::makeExpr(ast::Struct{ast.debuginfo, std::move(newargs)});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::StructAccess& ast) {
  return ast::makeExpr(ast::StructAccess{ast.debuginfo, rename(ast.stru), rename(ast.field)});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::ArrayInit& ast) {
  auto newargs =
      ast::transformArgs(ast.args, [&](ast::ExprPtr e) { return renamer.renameExpr(e); });
  return ast::makeExpr(ast::ArrayInit{ast.debuginfo, std::move(newargs)});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::ArrayAccess& ast) {
  return ast::makeExpr(ast::ArrayAccess{ast.debuginfo, rename(ast.array), rename(ast.index)});
}
ast::ExprPtr ExprRenameVisitor::operator()(ast::Tuple& ast) {
  auto newargs =
      ast::transformArgs(ast.args, [&](ast::ExprPtr e) { return renamer.renameExpr(e); });
  return ast::makeExpr(ast::ArrayInit{ast.debuginfo, std::move(newargs)});
}

ast::ExprPtr ExprRenameVisitor::operator()(ast::If& ast) {
  return ast::makeExpr(renamer.renameIf(ast));
}

using StatementRenameVisitor = SymbolRenamer::StatementRenameVisitor;

StatementPtr StatementRenameVisitor::operator()(ast::Fdef& ast) {
  auto newname = renamer.getNewName(ast.lvar.value);
  renamer.env->addToMap(ast.lvar.value, newname);
  auto newfun = renamer.expr_renamevisitor.renameLambda(ast.fun);
  auto newlvar = ast::Lvar{ast.lvar.debuginfo, newname, ast.lvar.type};
  return ast::makeStatement(ast::Fdef{ast.debuginfo, newlvar, newfun});
}

StatementPtr StatementRenameVisitor::operator()(ast::Assign& ast) {
  auto newname = renamer.getNewName(ast.lvar.value);
  renamer.env->addToMap(ast.lvar.value, newname);
  auto newrvar = renamer.renameExpr(ast.expr);
  auto newlvar = ast::Lvar{ast.lvar.debuginfo, newname, ast.lvar.type};
  return ast::makeStatement(ast::Assign{ast.debuginfo, std::move(newlvar), std::move(newrvar)});
}
StatementPtr StatementRenameVisitor::operator()(ast::Return& ast) {
  return ast::makeStatement(ast::Return{ast.debuginfo, renamer.renameExpr(ast.value)});
}
// StatementPtr StatementRenameVisitor::operator()(ast::Declaration& ast) {}
StatementPtr StatementRenameVisitor::operator()(ast::Time& ast) {
  return ast::makeStatement(
      ast::Time{ast.debuginfo, renamer.renameFcall(ast.fcall), renamer.renameExpr(ast.when)});
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
  auto newname = renamer.getNewName(ast.index.value);
  renamer.env->addToMap(ast.index.value, newname);
  auto newindex = ast::Lvar{ast.index.debuginfo, newname, ast.index.type};
  auto newstmts = renamer.renameBlock(ast.statements);
  renamer.env = renamer.env->parent_env;
  return ast::makeStatement(
      ast::For{ast.debuginfo, std::move(newindex), std::move(newiter), std::move(newstmts)});
}

}  // namespace mimium