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
  explicit Scheduler(std::shared_ptr<Runtime<TaskType>> runtime_i)
      : runtime(runtime_i), time(0) {}
  Scheduler(Scheduler& sch) = default;   // copy
  Scheduler(Scheduler&& sch) = default;  // move
  Scheduler& operator=(const Scheduler&) = default;
  Scheduler& operator=(Scheduler&&) = default;
  virtual ~Scheduler(){};
  virtual void start() = 0;
  virtual void stop() = 0;
  void incrementTime() {
    time++;
    if (!tasks.empty() && time > tasks.top().first) {
      executeTask(tasks.top().second);
    }
  };
  void addTask(int time, TaskType fn) { tasks.push(std::make_pair(time, fn)); };

 protected:
  std::shared_ptr<Runtime<TaskType>> runtime;
  using key_type = std::pair<int64_t, TaskType>;
  using queue_type = std::priority_queue<key_type, std::vector<key_type>,
                                         std::greater<key_type>>;
  int64_t time;
  queue_type tasks;
  virtual void executeTask(const TaskType& task)=0;
};

class SchedulerRT : public Scheduler<LLVMTaskType> {
 public:
  struct CallbackData {
    Scheduler<LLVMTaskType>* scheduler;
    std::shared_ptr<Runtime<LLVMTaskType>> runtime;
    CallbackData() : scheduler(), runtime(){};
  };
  explicit SchedulerRT(std::shared_ptr<Runtime<LLVMTaskType>> runtime_i)
      : Scheduler<LLVMTaskType>(runtime_i), audio() {
    userdata.scheduler = this;
    userdata.runtime = runtime;
  };
  virtual ~SchedulerRT(){};

  void start() override;
  void stop() override;

  inline int64_t getTime() { return time; }
  static int audioCallback(void* outputBuffer, void* inputBuffer,
                           unsigned int nBufferFrames, double streamTime,
                           RtAudioStreamStatus status, void* userdata);

 private:
 void executeTask(const LLVMTaskType& task) override;
  AudioDriver audio;
  CallbackData userdata;
};

class SchedulerSndFile : public Scheduler<LLVMTaskType> {
 public:
  explicit SchedulerSndFile(std::shared_ptr<Runtime<LLVMTaskType>> runtime_i);
  ~SchedulerSndFile();

  void start() override;
  void stop() override;

 private:
  SNDFILE* fp;
  SF_INFO sfinfo;
  double* buffer;
  void executeTask(const LLVMTaskType& task) override;
};

}  // namespace mimium