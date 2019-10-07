#include "runtime.hpp"
namespace mimium{
void Runtime::init(std::shared_ptr<ASTVisitor> _visitor) {
  visitor = _visitor;
  setupEnv();
}
void Runtime::setupEnv(){
    rootenv = std::make_shared<Environment>("root", nullptr);
  currentenv = rootenv;  // share
  currentenv->setVariable("dacL", 0);
  currentenv->setVariable("dacR", 0);
}
void Runtime::clear() {
  rootenv.reset();
  currentenv.reset();
  clearDriver();
  setupEnv();
}

void Runtime::start() {
  sch->start();
  running_status = true;
}

void Runtime::stop() {
  sch->stop();
  running_status = false;
}

void Runtime::loadSource(const std::string src) {
  driver.parsestring(src);
  loadAst(driver.getMainAst());
}
void Runtime::loadSourceFile(const std::string filename) {
  driver.parsefile(filename);
  loadAst(driver.getMainAst());
}

void Runtime::loadAst(AST_Ptr _ast) {
  auto ast = std::dynamic_pointer_cast<ListAST>(_ast);
  ast->accept(*visitor);
}
}