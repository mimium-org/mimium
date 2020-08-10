/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/ast.hpp"
#include "basic/ast_new.hpp"
#include "basic/environment.hpp"

namespace mimium {
using SymbolEnv = Environment<std::string>;

class AlphaConvertVisitor : public ASTVisitor {
 public:
  AlphaConvertVisitor();
  ~AlphaConvertVisitor() override;
  void init();
  void visit(OpAST& ast) override;
  void visit(ListAST& ast) override;
  void visit(NumberAST& ast) override;
  void visit(StringAST& ast) override;

  void visit(LvarAST& ast) override;
  void visit(RvarAST& ast) override;
  void visit(SelfAST& ast) override;
  void visit(AssignAST& ast) override;
  void visit(ArgumentsAST& ast) override;
  void visit(FcallArgsAST& ast) override;

  void visit(ArrayAST& ast) override;
  void visit(ArrayAccessAST& ast) override;
  void visit(FcallAST& ast) override;
  void visit(LambdaAST& ast) override;
  void visit(IfAST& ast) override;
  void visit(ReturnAST& ast) override;
  void visit(ForAST& ast) override;
  void visit(DeclarationAST& ast) override;
  // void visit(TimeAST& ast) override;
  void visit(StructAST& ast) override;
  void visit(StructAccessAST& ast) override;
  auto getResult() -> std::shared_ptr<ListAST>;

 private:
  std::stack<AST_Ptr> res_stack;
  template <class T>
  void defaultvisit(T& ast) {
    res_stack.push(std::make_unique<T>(ast));  // move by copy constructor;
  };
  template <class T, AST_ID ID>
  void listastvisit(AbstractListAST<std::shared_ptr<T>, ID>& ast) {
    auto newast = std::make_shared<
        AbstractListAST<std::shared_ptr<T>, ID>>();  // make empty args
    for (auto& elem : ast.getElements()) {
      elem->accept(*this);
      newast->appendAST(std::move(std::static_pointer_cast<T>(stackPopPtr())));
    }
    res_stack.push(std::move(newast));
  };

  auto createNewLVar(LvarAST& ast) -> std::unique_ptr<LvarAST>;

  std::shared_ptr<ListAST> currentcontext;
  std::shared_ptr<Environment<std::string>> env;
  int namecount;
  int envcount;
  auto stackPopPtr() -> AST_Ptr {
    auto r = res_stack.top();
    res_stack.pop();
    return r;
  }
};

// new alpha converter. It will not modify ast directly but create new copy of
// ast.
using AstPtr = std::shared_ptr<newast::Statements>;
using StatementPtr = std::shared_ptr<newast::Statement>;

class SymbolRenamer {
  explicit SymbolRenamer(std::shared_ptr<RenameEnvironment> env);

 public:
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
    newast::ExprPtr operator()(T& ast){
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
    StatementPtr operator()(T& ast) {
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