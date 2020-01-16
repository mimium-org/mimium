#pragma once
#include "ast.hpp"
#include "mir.hpp"
#include "type_infer_visitor.hpp"
#include "llvm_builtin_functions.hpp"

namespace mimium {
class KNormalizeVisitor : public ASTVisitor {
 public:
  explicit KNormalizeVisitor(std::shared_ptr<TypeInferVisitor> typeinfer_init);
  ~KNormalizeVisitor() override = default;
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
  mValue findVariable(std::string /*str*/) override { return 0.; }  //??

  std::shared_ptr<MIRblock> getResult();

 private:
  std::shared_ptr<TypeInferVisitor>
      typeinfer;  // to resolve anonymous function type;

  std::shared_ptr<MIRblock> rootblock;
  std::shared_ptr<MIRblock> currentblock;
  int var_counter;
  std::string makeNewName();
  std::string getVarName();
  bool isArgTime(FcallArgsAST& args);
  std::string tmpname;
  std::shared_ptr<ListAST> current_context;
  AST_Ptr insertAssign(AST_Ptr ast);
  void insertOverWrite(AST_Ptr body,const std::string& name);
  void insertAlloca(AST_Ptr body,const std::string& name);
  void insertRef(AST_Ptr body,const std::string& name);

  std::stack<std::string> res_stack_str;
  std::vector<std::string> lvar_list;
  std::string stackPopStr() {
    auto ret = res_stack_str.top();
    res_stack_str.pop();
    return ret;
  }
};

}  // namespace mimium