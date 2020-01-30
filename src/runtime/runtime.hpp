/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/helper_functions.hpp"
#include "runtime/runtime_defs.hpp"
namespace mimium{
class Scheduler;
class AudioDriver;
struct TaskType;

template <typename TaskType>
class Runtime {
 public:
  Runtime(std::string filename_i = "untitled") :waitc(){}

  virtual ~Runtime() = default;

  virtual void addScheduler()=0;

  virtual void start()=0;
  bool isrunning() { return running_status; };
  void stop() {
    running_status = false;
  };


  auto getScheduler() { return sch; };
  virtual void addAudioDriver(std::shared_ptr<AudioDriver> a)=0;;
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


}  // namespace mimium