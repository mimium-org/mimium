#include "runtime.hpp"
namespace mimium{

std::string Runtime::to_string(mValue v) {// where to place this
  return std::visit(overloaded{[](double v) { return std::to_string(v); },
                               [](std::vector<double> vec) {
                                 std::stringstream ss;
                                 ss << "[";
                                 int count = vec.size();
                                 for (auto& elem : vec) {
                                   ss << elem;
                                   count--;
                                   if (count > 0) ss << ",";
                                 }
                                 ss << "]";
                                 return ss.str();
                               },
                               [](std::string s) { return s; },
                               [](std::shared_ptr<AST> v) { return v->toString(); },
                               [](mClosure_ptr m){return m->toString();}},
                    v);
};

void Runtime::add_scheduler(bool issoundfile=false){
  if(issoundfile){
   sch = std::make_shared<SchedulerSndFile>(visitor);
  }else{
   sch = std::make_shared<SchedulerRT>(visitor);
  }
}

void Runtime::init(std::shared_ptr<ASTVisitor> _visitor) {
  visitor = _visitor;
  setupEnv();
}
void Runtime::setupEnv(){
    rootenv = std::make_shared<Environment>("root", nullptr);
  currentenv = rootenv;  // share
  currentenv->setVariable("dacL", 0.0);
  currentenv->setVariable("dacR", 0.0);
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