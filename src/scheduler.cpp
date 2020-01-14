#define _USE_MATH_DEFINES
#include "scheduler.hpp"

#include <cmath>
#include <cstdlib>
namespace mimium {

// void Scheduler::executeTask() {
//   tasks.top().second->accept(*interpreter);
//   // current_task_index->second->accept(*interpreter);
//   // const auto deleteitr = current_task_index;
//   // current_task_index++;
//   tasks.pop();
//   // tasks.erase(deleteitr);
//   if (time > tasks.top().first) {
//     executeTask();  // recursively execute if multiple tasks exist at the
//     same
//                     // time
//   }
// }

void SchedulerRT::start() {
  userdata.dspfn_ptr = runtime->getDspFn();
  userdata.dspfncls_ptr = runtime->getDspFnCls();
  audio.setCallback(SchedulerRT::audioCallback, &userdata);
  audio.start();
}

void SchedulerRT::stop() {
  isactive = false;
  {
    std::lock_guard<std::mutex> lock(waitc.mtx);
    waitc.isready = true;
  }
  waitc.cv.notify_all();  // notify to exit runtime;
}

int SchedulerRT::audioCallback(void* outputBuffer, void* inputBuffer,
                               unsigned int nBufferFrames, double streamTime,
                               RtAudioStreamStatus status, void* userData) {
  auto data = static_cast<CallbackData*>(userData);
  auto sch = data->scheduler;
  auto dspfn = data->dspfn_ptr;
  auto dspfn_cls = data->dspfncls_ptr;
  // auto interpreter = data->interpreter;
  if (sch->isactive) {
    double* outputBuffer_d = static_cast<double*>(outputBuffer);
    if (status)
      Logger::debug_log("Stream underflow detected!", Logger::WARNING);
    // Write interleaved audio data.
    for (int i = 0; i < nBufferFrames; i++) {
      auto shouldstop = sch->incrementTime();
      if (shouldstop) {
        sch->stop();        
        break;
      }
      if (dspfn != nullptr) {
          double res = dspfn((double)data->timeelapsed,dspfn_cls);
        outputBuffer_d[i * 2] = res;
        outputBuffer_d[i * 2 + 1] =res;
      }
      ++data->timeelapsed;
    }
  }
  return 0;
}
void SchedulerRT::executeTask(const LLVMTaskType& task) {
  // todo
  runtime->executeTask(task);
  tasks.pop();
  if (tasks.empty() && !runtime->hasDsp()) {
    runtime->stop();
  } else {
    if (time > tasks.top().first) {
      this->executeTask(tasks.top().second);
    }
  }
}

SchedulerSndFile::SchedulerSndFile(
    std::shared_ptr<Runtime<LLVMTaskType>> runtime_i, WaitController& waitc)
    : Scheduler(runtime_i, waitc) {
  sfinfo.channels = 2;
  sfinfo.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_16);
  sfinfo.samplerate = 48000;
  sfinfo.frames = 20 * 48000;  // temporary 20sec
  buffer = new double[sfinfo.channels * sfinfo.frames];
}

SchedulerSndFile::~SchedulerSndFile() { delete buffer; }

void SchedulerSndFile::start() {
  fp = sf_open("temp.wav", SFM_WRITE, &sfinfo);
  if (fp != nullptr) {
    std::runtime_error("opening file failed");
  };
  for (int i = 0; i < 20 * 48000; i++) {
    incrementTime();
    for (int chan = 0; chan < sfinfo.channels; chan++) {
      if (chan % 2) {
        //                 buffer[sfinfo.channels*i+chan] =
        // std::get<double>(interpreter->findVariable("dacL"));
      } else {
        //            buffer[sfinfo.channels*i+ chan] =
        // std::get<double>(interpreter->findVariable("dacR"));
      }
    }
  }
  auto res = sf_writef_double(fp, buffer, 20 * 48000);
  std::cout << res << std::endl;
  stop();
}

void SchedulerSndFile::stop() {
  if (sf_close(fp) != 0) {
    throw std::runtime_error("File is not correctly closed");
  } else {
    Logger::debug_log("File is closed", Logger::INFO);
    std::exit(0);
  }
}
void SchedulerSndFile::executeTask(const LLVMTaskType& task) {
  // todo
}

}  // namespace mimium