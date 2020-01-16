#pragma once
#include <memory>

#include "basic/ast.hpp"
#include "compiler/llvmgenerator.hpp"
#include "runtime/scheduler.hpp"
#include "compiler/type_infer_visitor.hpp"
#include "compiler/recursive_checker.hpp"

namespace mimium {
  using DspFnType= double(*)(double,void*);
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
  Runtime(std::string filename_i = "untitled") :waitc(){}

  virtual ~Runtime() = default;

  virtual void addScheduler(bool issoundfile) {
    if (issoundfile) {
      sch = std::make_shared<SchedulerSndFile>(this->shared_from_this(),waitc);
    } else {
      sch = std::make_shared<SchedulerRT>(this->shared_from_this(),waitc);
    }
  };

  virtual void start() {
    sch->start();
    running_status = true;
  };
  bool isrunning() { return running_status; };
  void stop() {
    sch->stop();
    running_status = false;
  };


  auto getScheduler() { return sch; };
  virtual void executeTask(const TaskType& task) = 0;

  virtual DspFnType getDspFn()=0;
  virtual void* getDspFnCls()=0;
  bool hasDsp(){return hasdsp;}
  bool hasDspCls(){return hasdspcls;}

 protected:
  std::shared_ptr<Scheduler<TaskType>> sch;
  bool running_status = false;
  bool hasdsp=false;
  bool hasdspcls =false;
  WaitController waitc;
};

class Runtime_LLVM : public Runtime<LLVMTaskType> {
 public:
  explicit Runtime_LLVM(std::string filename = "untitled.mmm",
                        bool isjit = true);

  ~Runtime_LLVM() = default;
  void addScheduler(bool issoundfile) override;

  void start() override;
  void executeModule(std::unique_ptr<llvm::Module> module);
  void executeTask(const LLVMTaskType& task) override;
  DspFnType getDspFn() override;
  void* getDspFnCls()override;
  auto& getJitEngine(){return *jitengine;}
  llvm::LLVMContext& getLLVMContext(){return jitengine->getContext();}

 private:

  DspFnType dspfn_address = nullptr;
  void* dspfn_cls_address=nullptr;
  std::unique_ptr<llvm::orc::MimiumJIT> jitengine;

};
}  // namespace mimium