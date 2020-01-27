#pragma once
#include "basic/helper_functions.hpp"
#include "runtime/scheduler/scheduler.hpp"

namespace mimium{
class Scheduler;
class SchedulerRT;
class SchedulerSndFile;
using DspFnType= double(*)(double,void*);



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