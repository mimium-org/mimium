#pragma once
#include <map>
#include <memory>
#include <queue>
#include <utility>

#include "ast.hpp"
#include "audiodriver.hpp"
#include "helper_functions.hpp"
#include "runtime.hpp"
#include "sndfile.h"

namespace mimium {
class Runtime;
template <typename TaskType>
class Scheduler {  // scheduler interface
 public:
  explicit Scheduler(std::shared_ptr<Runtime> runtime_i)
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
      executeTask();
    }
  };
  void addTask(int time, TaskType fn) { tasks.push(std::make_pair(time, fn)); };

 protected:
  std::shared_ptr<Runtime> runtime;
  using key_type = std::pair<int64_t, TaskType>;
  using queue_type = std::priority_queue<key_type, std::vector<key_type>,
                                         std::greater<key_type>>;
  int64_t time;
  queue_type tasks;
  virtual void executeTask()=0;
};

class SchedulerRT : public Scheduler<AST_Ptr> {
 public:
  struct CallbackData {
    Scheduler<AST_Ptr>* scheduler;
    std::shared_ptr<Runtime> runtime;
    CallbackData() : scheduler(), runtime(){};
  };
  explicit SchedulerRT(std::shared_ptr<Runtime> runtime_i)
      : Scheduler<AST_Ptr>(runtime_i), audio() {
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
 void executeTask() override;
  AudioDriver audio;
  CallbackData userdata;
};

class SchedulerSndFile : public Scheduler<AST_Ptr> {
 public:
  explicit SchedulerSndFile(std::shared_ptr<Runtime> runtime_i);
  ~SchedulerSndFile();

  void start() override;
  void stop() override;

 private:
  SNDFILE* fp;
  SF_INFO sfinfo;
  double* buffer;
  void executeTask() override;
};

}  // namespace mimium