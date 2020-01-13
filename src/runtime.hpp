#pragma once
#include <memory>

#include "alphaconvert_visitor.hpp"
#include "ast.hpp"
#include "closure_convert.hpp"
#include "driver.hpp"
#include "environment.hpp"
#include "knormalize_visitor.hpp"
#include "llvmgenerator.hpp"
#include "mididriver.hpp"
#include "scheduler.hpp"
#include "type_infer_visitor.hpp"
#include "recursive_checker.hpp"

namespace mimium {
  using dtodtype= double(*)(double);
struct LLVMTaskType {
  void* addresstofn;
  //int64_t tasktypeid;
  double arg;
  void* addresstocls;
};

template <typename TaskType>
class Scheduler;


class SchedulerRT;
class SchedulerSndFile;

template <typename TaskType>
class Runtime : public std::enable_shared_from_this<Runtime<TaskType>> {
 public:
  Runtime(std::string filename_i = "untitled") : filename(filename_i) ,waitc(){}

  virtual ~Runtime() = default;

  virtual void addScheduler(bool issoundfile) {
    if (issoundfile) {
      sch = std::make_shared<SchedulerSndFile>(this->shared_from_this(),waitc);
    } else {
      sch = std::make_shared<SchedulerRT>(this->shared_from_this(),waitc);
    }
  };
  virtual void init(){};
  void clear() { clearDriver(); };
  void clearDriver() { driver.clear(); };
  virtual void start() {
    sch->start();
    running_status = true;
  };
  bool isrunning() { return running_status; };
  void stop() {
    sch->stop();
    running_status = false;
  };
  AST_Ptr loadSource(std::string src) {
    driver.parsestring(src);
    return driver.getMainAst();
  };
  virtual AST_Ptr loadSourceFile(std::string filename) {
    driver.parsefile(filename);
    return driver.getMainAst();
  };
  Mididriver& getMidiInstance() { return midi; };

  AST_Ptr getMainAst() { return driver.getMainAst(); };
  auto getScheduler() { return sch; };
  virtual void executeTask(const TaskType& task) = 0;

  void setWorkingDirectory(const std::string path) {
    current_working_directory = path;
    driver.setWorkingDirectory(path);
  }
  virtual dtodtype getDspFn()=0;
  bool hasDsp(){return hasdsp;}

  std::string current_working_directory = "";

 protected:
  std::shared_ptr<Scheduler<TaskType>> sch;
  mmmpsr::MimiumDriver driver;
  Mididriver midi;
  std::string filename;
  bool running_status = false;
  bool hasdsp=false;
  WaitController waitc;
};

class Runtime_LLVM : public Runtime<LLVMTaskType> {
 public:
  explicit Runtime_LLVM(std::string filename = "untitled.mmm",
                        bool isjit = true);

  ~Runtime_LLVM() = default;
  void addScheduler(bool issoundfile) override;
  void checkRecursiveFuns(AST_Ptr _ast);
  AST_Ptr alphaConvert(AST_Ptr _ast);
  TypeEnv& typeInfer(AST_Ptr _ast);
  TypeEnv& getTypeEnv() { return typevisitor.getEnv(); };
  std::shared_ptr<MIRblock> kNormalize(AST_Ptr _ast);
  std::shared_ptr<MIRblock> closureConvert(std::shared_ptr<MIRblock> mir);
  auto llvmGenarate(std::shared_ptr<MIRblock> mir) -> std::string;
  void start() override;
  int execute();
  void executeTask(const LLVMTaskType& task) override;
  dtodtype getDspFn() override;

  AST_Ptr loadSourceFile(std::string filename) override;
  // virtual void loadAst(AST_Ptr _ast) override;

 private:
  AlphaConvertVisitor alphavisitor;
  TypeInferVisitor typevisitor;
  RecursiveChecker recursivechecker;
  std::shared_ptr<TypeInferVisitor> ti_ptr;
  KNormalizeVisitor knormvisitor;
  std::shared_ptr<ClosureConverter> closureconverter;
  std::shared_ptr<LLVMGenerator> llvmgenerator;
  dtodtype dspfn_address;
};
}  // namespace mimium