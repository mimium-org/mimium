#pragma once
#include <map>
#include <memory>
#include <queue>
#include <utility>

#include "ast.hpp"
#include "audiodriver.hpp"
#include "helper_functions.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "runtime.hpp"
#include "sndfile.h"

namespace mimium {
struct LLVMTaskType;

template <typename TaskType>
class Runtime;
template <typename TaskType>
class Scheduler {  // scheduler interface
 public:
  explicit Scheduler(std::shared_ptr<Runtime<TaskType>> runtime_i,WaitController& waitc)
      : runtime(runtime_i), time(0) ,waitc(waitc){}
  Scheduler(Scheduler& sch) = default;   // copy
  Scheduler(Scheduler&& sch) = default;  // move
  Scheduler& operator=(const Scheduler&) = default;
  Scheduler& operator=(Scheduler&&) = default;
  virtual ~Scheduler(){};
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual void stopAudioDriver()=0;

  bool hasTask() { return !tasks.empty(); }

  //tick the time and return if scheduler should be stopped
  bool incrementTime() {
    bool res = false;
    if (tasks.empty() && !runtime->hasDsp()) {
      res=true;
    } else {
      time+=1;
      if (time > tasks.top().first) {
        executeTask(tasks.top().second);
      }
    }
    return res;
  };
  // time,address to fun, arg(double), addresstoclosure,
  void addTask(double time, void* addresstofn, double arg,
               void* addresstocls) {
    TaskType task ={addresstofn, arg, addresstocls};
    tasks.emplace(static_cast<int64_t>(time), std::move(task));
  };
  // static auto getAddTaskAddress(){
  //     void(Scheduler<TaskType>::*ptr)(double,double(*)(double),double,double*)
  //     = &Scheduler<TaskType>::addTask; return ptr; }
  bool isactive=true;

 protected:
  WaitController& waitc;
  std::shared_ptr<Runtime<TaskType>> runtime;
  using key_type = std::pair<int64_t, TaskType>;
  struct Greater {
    bool operator()(const key_type& l, const key_type& r) const {
      return l.first > r.first;
    }
  };

  using queue_type =
      std::priority_queue<key_type, std::vector<key_type>, Greater>;
  int64_t time;
  queue_type tasks;
  virtual void executeTask(const TaskType& task) = 0;
};

class SchedulerRT : public Scheduler<LLVMTaskType> {
 public:
  struct CallbackData {
    Scheduler<LLVMTaskType>* scheduler;
    // std::shared_ptr<Runtime<LLVMTaskType>> runtime;
    double (*dspfnptr)(double){};
    int64_t timeelapsed;
    CallbackData() : scheduler(), timeelapsed(0){};
  };
  explicit SchedulerRT(std::shared_ptr<Runtime<LLVMTaskType>> runtime_i,WaitController& waitc)
      : Scheduler<LLVMTaskType>(runtime_i,waitc), audio() {
    userdata.scheduler = this;
  };
  virtual ~SchedulerRT(){};

  void start() override;
  void stop() override;

  inline int64_t getTime() { return time; }
  static int audioCallback(void* outputBuffer, void* inputBuffer,
                           unsigned int nBufferFrames, double streamTime,
                           RtAudioStreamStatus status, void* userdata);
  void stopAudioDriver()override {audio.stop();}
 private:
  void executeTask(const LLVMTaskType& task) override;
  AudioDriver audio;
  CallbackData userdata;
};

class SchedulerSndFile : public Scheduler<LLVMTaskType> {
 public:
  explicit SchedulerSndFile(std::shared_ptr<Runtime<LLVMTaskType>> runtime_i,WaitController& waitc);
  ~SchedulerSndFile();

  void start() override;
  void stop() override;
  void stopAudioDriver() override{};

 private:
  SNDFILE* fp;
  SF_INFO sfinfo;
  double* buffer;
  void executeTask(const LLVMTaskType& task) override;
};

}  // namespace mimium