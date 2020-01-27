#pragma once
#include "basic/ast.hpp"
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
  void visit(TimeAST& ast) override;
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

}  // namespace mimium