#pragma once
#include <cmath>
#include <unordered_map>
#include <variant>
#include "ast.hpp"
#include "builtin_functions.hpp"
#include "helper_functions.hpp"
#include "runtime.hpp"

namespace mimium {

class InterpreterVisitor
    : public ASTVisitor,public std::enable_shared_from_this<InterpreterVisitor> {
 public:
  InterpreterVisitor() =default;
  ~InterpreterVisitor(){};
  // void add_scheduler() { sch = std::make_shared<Scheduler>(this); };
  // void init();
  // void clear();
  // inline void clearDriver() { driver.clear(); };
  // void start();
  // inline bool isrunning() { return running_status; };
  // void stop();
  // void loadSource(const std::string src);
  // void loadSourceFile(const std::string filename);
  // void loadAst(AST_Ptr _ast);

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
  void loadAst(AST_Ptr _ast);
  static std::string to_string(mValue v);
  void init();
  double get_as_double(mValue v);
  void doForVisitor(mValue v, std::string iname, AST_Ptr expression);
  inline auto& getVstack() { return res_stack; };
  mimium::Runtime& getRuntime();
 private:
  mimium::Runtime runtime;
  bool assertArgumentsLength(std::vector<AST_Ptr>& args, int length);
};
}  // namespace mimium