#pragma once
#include "basic/ast.hpp"
#include "compiler/ffi.hpp"
#include "basic/helper_functions.hpp"
#include "basic/type.hpp"
// type inference ... assumed to be visited after finished alpha-conversion(each
// variable has unique name regardless its scope)

namespace mimium {

class TypeInferVisitor : public ASTVisitor {
 public:
  TypeInferVisitor();
  ~TypeInferVisitor() override = default;
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

  bool typeCheck(types::Value& lt, types::Value& rt);
  bool unify(types::Value& lt, types::Value& rt);

  bool unify(std::string lname, std::string rname);
  bool unify(std::string lname, types::Value& rt);
  bool unifyArg(types::Value& target, types::Value& realarg);

  TypeEnv& getEnv() { return typeenv; };
  types::Value getLastType() { return res_stack.top(); }
  types::Value stackPop(){
    auto res = res_stack.top();
    res_stack.pop();
    return res;
  }
  std::string tmpfname;

  TypeEnv& infer(AST_Ptr toplevel);
  void replaceTypeVars();
 private:
  std::stack<types::Value> res_stack;
  static bool checkArg(types::Value& fnarg, types::Value& givenarg);
  //hold value for infer type of "self"
  std::optional<types::Value> current_return_type;
  TypeEnv typeenv;
  bool has_return;
};
}  // namespace mimium