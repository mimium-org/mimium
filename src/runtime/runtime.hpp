#pragma once
#include "basic/helper_functions.hpp"
#include "jit_engine.hpp"
#include "runtime/scheduler.hpp"
#include "runtime/backend/rtaudio/driver_rtaudio.hpp"
namespace mimium{
class Scheduler;
class SchedulerRT;
class SchedulerSndFile;
  using DspFnType= double(*)(double,void*);
struct LLVMTaskType {
  void* addresstofn;
  //int64_t tasktypeid;
  double arg;
  void* addresstocls;
};
}

namespace mimium {


template <typename TaskType>
class Runtime {
 public:
  Runtime(std::string filename_i = "untitled") :waitc(){}

  virtual ~Runtime() = default;

  virtual void addScheduler(bool issoundfile)=0;

  virtual void start()=0;
  bool isrunning() { return running_status; };
  void stop() {
    running_status = false;
  };


  auto getScheduler() { return sch; };

  virtual DspFnType getDspFn()=0;
  virtual void* getDspFnCls()=0;
  bool hasDsp(){return hasdsp;}
  bool hasDspCls(){return hasdspcls;}

 protected:
  std::shared_ptr<Scheduler> sch;
  bool running_status = false;
  bool hasdsp=false;
  bool hasdspcls =false;
  WaitController waitc;
};

class Runtime_LLVM : public Runtime<LLVMTaskType> , public std::enable_shared_from_this<Runtime_LLVM>{
 public:
  explicit Runtime_LLVM(std::string filename = "untitled.mmm",
                        bool isjit = true);

  ~Runtime_LLVM() = default;
void addScheduler(bool is_soundfile)override;
  void start() override;
  DspFnType getDspFn() override;
  void* getDspFnCls()override;

  void executeModule(std::unique_ptr<llvm::Module> module);
  auto& getJitEngine(){return *jitengine;}
  llvm::LLVMContext& getLLVMContext(){return jitengine->getContext();}

 private:

  DspFnType dspfn_address = nullptr;
  void* dspfn_cls_address=nullptr;
  std::unique_ptr<llvm::orc::MimiumJIT> jitengine;

};
}  // namespace mimium