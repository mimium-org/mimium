#pragma once
#include "ast.hpp"
#include "environment.hpp"

namespace mimium {

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
  void visit(AssignAST& ast) override;
  void visit(ArgumentsAST& ast) override;
  void visit(ArrayAST& ast) override;
  void visit(ArrayAccessAST& ast) override;
  void visit(FcallAST& ast) override;
  void visit(LambdaAST& ast) override;
  void visit(IfAST& ast) override;
  void visit(ReturnAST& ast) override;
  void visit(ForAST& ast) override;
  void visit(DeclarationAST& ast) override;
  void visit(TimeAST& ast) override;
  void visit(StructAST& ast)override;
  void visit(StructAccessAST& ast)override;
  auto getResult() -> std::shared_ptr<ListAST>;
  mValue findVariable(std::string str)override{return 0.;};
 private:
  template <class T>
  void defaultvisit(T& ast) {
    res_stack.push(std::make_unique<T>(ast));  // move by copy constructor;
  };
  template <class MYAST>
  void listastvisit(MYAST& ast) {
    auto newast = std::make_shared<MYAST>();  // make empty args
    for (auto& elem : ast.getElements()) {
      elem->accept(* this);
      newast->appendAST(std::move(stackPopPtr()));
    }
    res_stack.push(std::move(newast));
  };
  std::shared_ptr<ListAST> currentcontext;
  std::shared_ptr<Environment> env;
  int namecount;
  int envcount;
  auto stackPopPtr() -> AST_Ptr { return std::get<AST_Ptr>(stack_pop()); }
};

}  // namespace mimium