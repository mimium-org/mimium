/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <memory>
#include <utility>

#include "basic/ast.hpp"
#include "basic/environment.hpp"
namespace mimium {

// new alpha converter. It will not modify ast directly but create new copy of
// ast.
using AstPtr = std::shared_ptr<ast::Statements>;
using StatementPtr = std::shared_ptr<ast::Statement>;

class SymbolRenamer {
 public:
  SymbolRenamer();
  explicit SymbolRenamer(std::shared_ptr<RenameEnvironment> env);
  AstPtr rename(ast::Statements& ast);

  struct ExprRenameVisitor : public VisitorBase<ast::ExprPtr> {
    explicit ExprRenameVisitor(SymbolRenamer& parent) : renamer(parent){};
    SymbolRenamer& renamer;
    ast::ExprPtr operator()(ast::Op& ast);
    ast::ExprPtr operator()(ast::Number& ast);
    ast::ExprPtr operator()(ast::String& ast);
    ast::ExprPtr operator()(ast::Rvar& ast);
    ast::ExprPtr operator()(ast::Self& ast);
    ast::LambdaArgs renameLambdaArgs(ast::LambdaArgs& ast);
    ast::Lambda renameLambda(ast::Lambda& ast);
    ast::ExprPtr operator()(ast::Lambda& ast);

    ast::ExprPtr operator()(ast::Fcall& ast);
    ast::ExprPtr operator()(ast::Struct& ast);
    ast::ExprPtr operator()(ast::StructAccess& ast);
    ast::ExprPtr operator()(ast::ArrayInit& ast);
    ast::ExprPtr operator()(ast::ArrayAccess& ast);
    ast::ExprPtr operator()(ast::Tuple& ast);

    ast::ExprPtr operator()(ast::If& ast);

    ast::Block renameBlock(ast::Block& ast);
    ast::ExprPtr operator()(ast::Block& ast);
    ast::ExprPtr rename(ast::ExprPtr ast) {
      return std::visit(*this, *ast);
    }
  };
  struct StatementRenameVisitor : public VisitorBase<StatementPtr> {
    explicit StatementRenameVisitor(SymbolRenamer& parent) : renamer(parent){};
    SymbolRenamer& renamer;
    StatementPtr operator()(ast::Assign& ast);
    StatementPtr operator()(ast::Fdef& ast);

    StatementPtr operator()(ast::Return& ast);
    // StatementPtr operator()(ast::Declaration& ast);
    StatementPtr operator()(ast::Time& ast);
    StatementPtr operator()(ast::Fcall& ast);
    StatementPtr operator()(ast::For& ast);
    
    StatementPtr rename(StatementPtr ast) {
      return std::visit(*this, *ast);
    }
  };
  StatementPtr renameStatement(StatementPtr stmt){
    return statement_renamevisitor.rename(stmt);
  }
  ast::ExprPtr renameExpr(ast::ExprPtr expr){
    return expr_renamevisitor.rename(expr);
  }
  ast::FcallArgs renameFcallArgs(ast::FcallArgs& ast);

  ast::Fcall renameFcall(ast::Fcall& ast);
 private:
  ExprRenameVisitor expr_renamevisitor{*this};
  StatementRenameVisitor statement_renamevisitor{*this};
  std::shared_ptr<RenameEnvironment> env;
  uint64_t namecount = 0;
  std::string getNewName(std::string const& name);
  std::string searchFromEnv(std::string const& name);
};

}  // namespace mimium