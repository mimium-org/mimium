#pragma once

#include "basic/ast.hpp"
#include "basic/helper_functions.hpp"
#include "runtime/audiodriver.hpp"
#include "runtime/runtime.hpp"
#include "sndfile.h"

namespace mimium {
struct LLVMTaskType;

class AudioDriver;
class Runtime_LLVM;

class Scheduler {  // scheduler interface
 public:
  explicit Scheduler(std::shared_ptr<Runtime_LLVM> runtime_i,
                     WaitController& waitc)
      : waitc(waitc), runtime(runtime_i), time(0) {}

  virtual ~Scheduler()=default;
  virtual void start() = 0;
  virtual void stop() = 0;
  void haltRuntime();

  bool hasTask() { return !tasks.empty(); }

  // tick the time and return if scheduler should be stopped
  bool incrementTime();

  // time,address to fun, arg(double), addresstoclosure,
  void addTask(double time, void* addresstofn, double arg, void* addresstocls);

  virtual void setDsp(DspFnType fn,void* cls)=0;

  bool isactive = true;
  Runtime_LLVM& getRuntime() { return *runtime; };
  auto getTime() { return time; };

 protected:
  WaitController& waitc;
  std::shared_ptr<Runtime_LLVM> runtime;
  using key_type = std::pair<int64_t, LLVMTaskType>;
  struct Greater {
    bool operator()(const key_type& l, const key_type& r) const;
  };

  using queue_type =
      std::priority_queue<key_type, std::vector<key_type>, Greater>;
  int64_t time;
  queue_type tasks;
  virtual void executeTask(const LLVMTaskType& task);
};
using DspFnType = double (*)(double, void*);

class SchedulerRT : public Scheduler {
 public:
  explicit SchedulerRT(std::shared_ptr<Runtime_LLVM> runtime_i,
                       WaitController& waitc);
  ~SchedulerRT()override =default;

  void start() override;
  void stop() override;
  void setDsp(DspFnType fn,void* cls)override;
 private:
  std::shared_ptr<AudioDriver> audio;
};

class SchedulerSndFile : public Scheduler {
 public:
  explicit SchedulerSndFile(std::shared_ptr<Runtime_LLVM> runtime_i,
                            WaitController& waitc);
  ~SchedulerSndFile()=default;

  void start() override;
  void stop() override;
  void setDsp(DspFnType fn,void* cls)override{
// later
  }
 private:
  SNDFILE* fp;
  SF_INFO sfinfo;
  std::vector<double> buffer;
};

}  // namespace mimium