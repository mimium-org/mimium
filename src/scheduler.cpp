#include "scheduler.hpp"

void mimium::Scheduler::start() {
  audio.setCallback(mimium::Scheduler::audioCallback, this);
  audio.start();
}

void mimium::Scheduler::stop() { audio.stop(); }
void mimium::Scheduler::incrementTime() {
  time++;
  if (time > current_task_index->first) {
    executeTask();
  }
}
void mimium::Scheduler::executeTask() {
  interpreter->interpretListAst(current_task_index->second);
  const auto deleteitr = current_task_index;
  current_task_index++;
  tasks.erase(deleteitr);
  if (time > current_task_index->first) {
    executeTask();  // recursively execute if multiple tasks exist at the same
                    // time
  }
}

void mimium::Scheduler::addTask(int time, AST_Ptr fn) {
  // fn->set_time(-1); //remove time to execute
  tasks.insert(std::make_pair(time, fn));
  if (tasks.size() == 1) {
    current_task_index = tasks.begin();
    nexttask_time = time;
  }
}
int mimium::Scheduler::audioCallback(void* outputBuffer, void* inputBuffer,
                                     unsigned int nBufferFrames,
                                     double streamTime,
                                     RtAudioStreamStatus status,
                                     void* userData) {
  auto sch = (mimium::Scheduler*)userData;
  double* outputBuffer_d[nBufferFrames];
  if (status) Logger::debug_log("Stream underflow detected!", Logger::WARNING);
  // Write interleaved audio data.
  for (int i = 0; i < nBufferFrames; i++) {
    sch->incrementTime();
    outputBuffer_d[i] = 0;
  }
  outputBuffer = outputBuffer_d;
  return 0;
}