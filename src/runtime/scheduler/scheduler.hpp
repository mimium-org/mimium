/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#define LLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING 1

#include <queue>
#include <utility>

#include "basic/helper_functions.hpp"
#include "runtime/backend/audiodriver.hpp"

#include "runtime/runtime.hpp"
// #include "sndfile.h"

namespace mimium {
struct TaskType {
  void* addresstofn;
  //int64_t tasktypeid;
  double arg;
  void* addresstocls;
};

class AudioDriver;
using LLVMRuntime = Runtime<TaskType>;

class Scheduler {  // scheduler interface
 public:
  explicit Scheduler(std::shared_ptr<LLVMRuntime> runtime_i,
                     WaitController& waitc)
      : waitc(waitc), runtime(std::move(runtime_i)), time(0) {}

  virtual ~Scheduler()=default;
  virtual void start();
  virtual void stop();
  void haltRuntime();

  bool hasTask() { return !tasks.empty(); }

  // tick the time and return if scheduler should be stopped
  bool incrementTime();

  // time,address to fun, arg(double), addresstoclosure,
  void addTask(double time, void* addresstofn, double arg, void* addresstocls);

  virtual void setDsp(DspFnType fn);
  virtual void setDsp_ClsAddress(void* address);
  virtual void setDsp_MemobjAddress(void* address);


  bool isactive = true;
  LLVMRuntime& getRuntime() { return *runtime; };
  auto getTime() { return time; };

  void addAudioDriver(std::shared_ptr<AudioDriver> a);

 protected:
  WaitController& waitc;
  std::shared_ptr<LLVMRuntime> runtime;
  std::shared_ptr<AudioDriver> audio;

  using key_type = std::pair<int64_t, TaskType>;
  struct Greater {
    bool operator()(const key_type& l, const key_type& r) const;


  };

  using queue_type =
      std::priority_queue<key_type, std::vector<key_type>, Greater>;
  int64_t time;
  queue_type tasks;
  virtual void executeTask(const TaskType& task);
};

// class SchedulerSndFile : public Scheduler {
//  public:
//   explicit SchedulerSndFile(std::shared_ptr<LLVMRuntime> runtime_i,
//                             WaitController& waitc);
//   ~SchedulerSndFile()=default;

//   void start() override;
//   void stop() override;
//   void setDsp(DspFnType fn,void* cls)override{
// // later
//   }
//  private:
//   SNDFILE* fp;
//   SF_INFO sfinfo;
//   std::vector<double> buffer;
// };

}  // namespace mimium