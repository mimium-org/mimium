#pragma once
#include "ast.hpp"
#include "llvm_builtin_functions.hpp"
#include "helper_functions.hpp"
#include "type.hpp"
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

  mValue findVariable(std::string /*str*/) override { return 0.; }  //??
  TypeEnv& getEnv() { return typeenv; };
  types::Value getLastType() { return res_stack; }
  std::string_view tmpfname;

 private:
  types::Value res_stack;
  static bool checkArg(const types::Value& fnarg, const types::Value& givenarg);
  TypeEnv typeenv;
  bool has_return;
};
}  // namespace mimium