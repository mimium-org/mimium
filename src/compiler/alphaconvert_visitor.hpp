/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/ast_new.hpp"
#include "basic/environment.hpp"
#include <memory>
#include <utility>
namespace mimium {

// new alpha converter. It will not modify ast directly but create new copy of
// ast.
using AstPtr = std::shared_ptr<newast::Statements>;
using StatementPtr = std::shared_ptr<newast::Statement>;

class SymbolRenamer {

 public:
  SymbolRenamer();
  explicit SymbolRenamer(std::shared_ptr<RenameEnvironment> env);
  AstPtr rename(newast::Statements& ast);

  struct ExprRenameVisitor {
    explicit ExprRenameVisitor(SymbolRenamer& parent) : renamer(parent){};
    SymbolRenamer& renamer;
    newast::ExprPtr operator()(newast::Op& ast);
    newast::ExprPtr operator()(newast::Number& ast);
    newast::ExprPtr operator()(newast::String& ast);
    newast::ExprPtr operator()(newast::Rvar& ast);
    newast::ExprPtr operator()(newast::Self& ast);
    newast::ExprPtr operator()(newast::Lambda& ast);
    newast::ExprPtr operator()(newast::Fcall& ast);
    newast::ExprPtr operator()(newast::Time& ast);
    newast::ExprPtr operator()(newast::Struct& ast);
    newast::ExprPtr operator()(newast::StructAccess& ast);
    newast::ExprPtr operator()(newast::ArrayInit& ast);
    newast::ExprPtr operator()(newast::ArrayAccess& ast);
    newast::ExprPtr operator()(newast::Tuple& ast);
    template <typename T>
    newast::ExprPtr operator()(Rec_Wrap<T>& ast) {
      // default action
      return (*this)(ast.getraw());
    }
    //in case missing to list asts
    template<typename T>
    newast::ExprPtr operator()(T&  /*ast*/){
      static_assert(true, "missing some visitor functions for ExprRenameVisitor");
    }
  };
  struct StatementRenameVisitor {
    explicit StatementRenameVisitor(SymbolRenamer& parent) : renamer(parent){};
    SymbolRenamer& renamer;
    StatementPtr operator()(newast::Assign& ast);
    StatementPtr operator()(newast::Return& ast);
    // StatementPtr operator()(newast::Declaration& ast);
    StatementPtr operator()(newast::For& ast);
    StatementPtr operator()(newast::If& ast);
    StatementPtr operator()(newast::ExprPtr& ast);
    template <typename T>
    StatementPtr operator()(Rec_Wrap<T>& ast) {
      return (*this)(ast.getraw());
    }
    template <typename T>
    StatementPtr operator()(T&  /*ast*/) {
      static_assert(true, "missing some visitor functions for StatementRenameVisitor");
    }
  };

 private:
  ExprRenameVisitor expr_renamevisitor{*this};
  StatementRenameVisitor statement_renamevisitor{*this};
  std::shared_ptr<RenameEnvironment> env;
  uint64_t namecount = 0;
  std::string getNewName(std::string const& name);
  std::string searchFromEnv(std::string const& name);
};

}  // namespace mimium