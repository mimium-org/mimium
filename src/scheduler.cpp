#define _USE_MATH_DEFINES
#include <cmath>

#include "scheduler.hpp"
#include <cstdlib>
namespace mimium{

void Scheduler::incrementTime() {
  time++;
  if (!tasks.empty() && time > tasks.top().first) {
    executeTask();
  }
}
void Scheduler::executeTask() {
  tasks.top().second->accept(*interpreter);
  // current_task_index->second->accept(*interpreter);
  // const auto deleteitr = current_task_index;
  // current_task_index++;
  tasks.pop();
  // tasks.erase(deleteitr);
  if (time > tasks.top().first) {
    executeTask();  // recursively execute if multiple tasks exist at the same
                    // time
  }
}

void Scheduler::addTask(int time, AST_Ptr fn) {
  // fn->set_time(-1); //remove time to execute
  tasks.push(std::make_pair(time,fn));
  // tasks.insert(std::make_pair(time, fn));
  // if (tasks.size() == 1) {
  //   current_task_index = tasks.begin();
  //   nexttask_time = time;
  // }
}


void SchedulerRT::start() {
  audio.setCallback(SchedulerRT::audioCallback,&userdata );
  audio.start();
}

void SchedulerRT::stop() { audio.stop(); }

int SchedulerRT::audioCallback(void* outputBuffer, void* inputBuffer,
                                     unsigned int nBufferFrames,
                                     double streamTime,
                                     RtAudioStreamStatus status,
                                     void* userData) {
  auto data = static_cast<CallbackData*>(userData);
  auto sch = data->scheduler;
  auto interpreter = data->interpreter;
  double* outputBuffer_d =static_cast<double*>(outputBuffer);
  if (status) Logger::debug_log("Stream underflow detected!", Logger::WARNING);
  // Write interleaved audio data.
  // double d =0;
  // double d2=0;
  for (int i = 0; i < nBufferFrames; i++) {
    sch->incrementTime();

    outputBuffer_d[i*2] = std::get<double>(interpreter->findVariable("dacL"));
    outputBuffer_d[i*2+1] =std::get<double>(interpreter->findVariable("dacR"));

  }
  return 0;
}

}//namespace mimium