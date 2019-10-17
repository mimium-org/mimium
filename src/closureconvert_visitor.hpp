#include "ast.hpp"
#include "environment.hpp"
namespace mimium {

class ClosureConvertVisitor : public ASTVisitor {
 public:
  ClosureConvertVisitor();
  ~ClosureConvertVisitor();
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
  mValue findVariable(std::string str) override;
  private:
  bool isFreeVariable(std::string str);
  void movetoTopLevel_Closure();

  bool is_closed_function;
  std::unordered_map<std::string,std::string> lambdaname_map;
  std::shared_ptr<Environment> env;
  std::shared_ptr<ArgumentsAST> args;
  ListAST& toplevel;
  int closurecount;
};

}  // namespace mimium