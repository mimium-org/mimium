#include "runtime.hpp"
#include "runtime/backend/audiodriver.hpp"
#include "runtime/executionengine/executionengine.hpp"

namespace mimium {
Runtime::Runtime(std::unique_ptr<AudioDriver> a, std::unique_ptr<ExecutionEngine> e)
    : audiodriver(std::move(a)), executionengine(std::move(e)) {}

void Runtime::runMainFun() { this->hasdsp = executionengine->runMainFunction(this); }

void Runtime::start() {
  executionengine->preStart();

  auto& sch = audiodriver->getScheduler();
  if (hasdsp || sch.hasTask()) {
    audiodriver->setup(audiodriver->getDefaultAudioParameter(std::nullopt, std::nullopt));
    audiodriver->start();
    {
      auto& waitc = sch.getWaitController();
      std::unique_lock<std::mutex> uniq_lk(waitc.mtx);
      // aynchronously wait until scheduler stops
      waitc.cv.wait(uniq_lk, [&]() { return waitc.isready; });
    }
  }
}

AudioDriver& Runtime::getAudioDriver() { return *audiodriver; }

void Runtime::pushMalloc(void* address, size_t size) {
  malloc_container.emplace_back(address, size);
}
}  // namespace mimium

extern "C" {
void setDspParams(void* runtimeptr, void* dspfn, void* clsaddress, void* memobjaddress,
                  int in_numchs, int out_numchs) {
  auto* runtime = static_cast<mimium::Runtime*>(runtimeptr);
  auto& audiodriver = runtime->getAudioDriver();
  auto p = std::make_unique<mimium::DspFnInfos>(
      mimium::DspFnInfos{reinterpret_cast<mimium::DspFnPtr>(dspfn), clsaddress, memobjaddress,
                         in_numchs, out_numchs});  // NOLINT
  audiodriver.setDspFnInfos(std::move(p));
}

NO_SANITIZE void addTask(void* runtimeptr, double time, void* addresstofn, double arg) {
  auto* runtime = static_cast<mimium::Runtime*>(runtimeptr);
  mimium::Scheduler& sch = runtime->getAudioDriver().getScheduler();
  sch.addTask(time, addresstofn, arg, nullptr);
}
NO_SANITIZE void addTask_cls(void* runtimeptr, double time, void* addresstofn, double arg,
                             void* addresstocls) {
  auto* runtime = static_cast<mimium::Runtime*>(runtimeptr);
  mimium::Scheduler& sch = runtime->getAudioDriver().getScheduler();
  sch.addTask(time, addresstofn, arg, addresstocls);
}
double mimium_getnow(void* runtimeptr) {
  auto* runtime = static_cast<mimium::Runtime*>(runtimeptr);
  return (double)runtime->getAudioDriver().getScheduler().getTime();
}

// TODO(tomoya) ideally we need to move this to base runtime library
void* mimium_malloc(void* runtimeptr, size_t size) {
  auto* runtime = static_cast<mimium::Runtime*>(runtimeptr);
  void* address = malloc(size);  // NOLINT
  runtime->pushMalloc(address, size);
  return address;
}
}