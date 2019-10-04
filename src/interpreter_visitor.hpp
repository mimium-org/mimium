#pragma once
#include <cmath>
#include <memory>
#include <string>
#include <stack>
#include <unordered_map>
#include <variant>
#include "ast.hpp"
#include "builtin_functions.hpp"
#include "closure.hpp"
#include "driver.hpp"
#include "helper_functions.hpp"
#include "mididriver.hpp"
#include "scheduler.hpp"

namespace mimium {
class Scheduler;  // forward

class InterpreterVisitor
    : public ASTVisitor,
      public std::enable_shared_from_this<InterpreterVisitor> {
 public:
  explicit InterpreterVisitor() { midi.init(); }
  mValue findVariable(std::string str) {  // fortest
    auto it = arguments.find(str);
    if (it != arguments.end()) {
      return it->second;
    } else {
      return currentenv->findVariable(str);
    }
  }
  void add_scheduler() { sch = std::make_shared<Scheduler>(this); };
  void init();
  void clear();
  inline void clearDriver() { driver.clear(); };
  void start();
  inline bool isrunning() { return running_status; };
  void stop();
  void loadSource(const std::string src);
  void loadSourceFile(const std::string filename);
  void loadAst(AST_Ptr _ast);

  void visit(OpAST& ast);
  void visit(ListAST& ast);
  void visit(NumberAST& ast);
  void visit(SymbolAST& ast);
  void visit(AssignAST& ast);
  void visit(ArgumentsAST& ast);
  void visit(ArrayAST& ast);
  void visit(ArrayAccessAST& ast);
  void visit(FcallAST& ast);
  void visit(LambdaAST& ast);
  void visit(IfAST& ast);
  void visit(ReturnAST& ast);
  void visit(ForAST& ast);
  void visit(DeclarationAST& ast);
  void visit(TimeAST& ast);
  inline Mididriver& getMidiInstance() { return midi; };
  inline std::shared_ptr<Environment> getCurrentEnv() { return currentenv; };
  inline AST_Ptr getMainAst() { return driver.getMainAst(); };

  static std::string to_string(mValue v);
  void setWorkingDirectory(const std::string path) {
    current_working_directory = path;
    driver.setWorkingDirectory(path);
  }
  double get_as_double(mValue v);
  void doForVisitor(mValue v, std::string iname, AST_Ptr expression);
  auto& getVstack(){return vstack;};
 private:
  std::shared_ptr<Environment> rootenv;
  std::shared_ptr<Environment> currentenv;
  std::map<std::string, mValue> arguments;
  std::string currentNS;
  std::shared_ptr<Scheduler> sch;
  std::stack<mValue> vstack;
  mmmpsr::MimiumDriver driver;
  Mididriver midi;
  // Builtin* builtin_functions;
  // Logger logger;
  std::string current_working_directory = "";
  bool running_status = false;
  bool assertArgumentsLength(std::vector<AST_Ptr>& args, int length);
};
}  // namespace mimium