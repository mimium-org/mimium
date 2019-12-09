#pragma once
#include "ast.hpp"
#include "mididriver.hpp"
#include "scheduler.hpp"
#include "closure.hpp"
#include "driver.hpp"

namespace mimium {
class Scheduler;//forward
class Runtime {
 public:
  explicit Runtime(){ 
    midi.init(); }

  virtual ~Runtime(){};
  mValue findVariable(std::string str) {  // fortest
    auto it = arguments.find(str);
    if (it != arguments.end()) {
      return it->second;
    } else {
      return currentenv->findVariable(str);
    }
  }
  void add_scheduler(bool issoundfile);
  void init(std::shared_ptr<ASTVisitor> _visitor);
  void setupEnv();
  void clear();
  inline void clearDriver() { driver.clear(); };
  void start();
  inline bool isrunning() { return running_status; };
  void stop();
  void loadSource(const std::string src);
  void loadSourceFile(const std::string filename);
  virtual void loadAst(AST_Ptr _ast);
  inline Mididriver& getMidiInstance() { return midi; };
  inline std::shared_ptr<InterpreterEnv> getCurrentEnv() { return currentenv; };
  inline void setCurrentEnv(std::shared_ptr<InterpreterEnv> env){currentenv = env;};
  inline AST_Ptr getMainAst() { return driver.getMainAst(); };
  inline auto getScheduler(){return sch;};
  static std::string to_string(mValue v);
  void setWorkingDirectory(const std::string path) {
    current_working_directory = path;
    driver.setWorkingDirectory(path);
  }
  std::string current_working_directory = "";
 protected:
  std::shared_ptr<InterpreterEnv> rootenv;
  std::shared_ptr<InterpreterEnv> currentenv;
  std::string currentNS;
  std::shared_ptr<Scheduler> sch;
  mmmpsr::MimiumDriver driver;
  Mididriver midi;
  std::map<std::string, mValue> arguments;
  std::shared_ptr<ASTVisitor> visitor;
  bool running_status = false;
};
}  // namespace mimium