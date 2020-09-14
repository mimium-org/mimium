/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "runtime/JIT/runtime_jit.hpp"

extern "C" {
mimium::Runtime_LLVM* global_runtime;

void setDspParams(void* dspfn, void* clsaddress, void* memobjaddress) {
  auto audiodriver = global_runtime->getAudioDriver();
  audiodriver->setDspFn(reinterpret_cast<mimium::DspFnType>(dspfn));
  audiodriver->setDspClsAddress(clsaddress);
  audiodriver->setDspMemObjAddress(memobjaddress);
}

NO_SANITIZE void addTask(double time, void* addresstofn, double arg) {
  mimium::Scheduler& sch = global_runtime->getAudioDriver()->getScheduler();
  sch.addTask(time, addresstofn, arg, nullptr);
}
NO_SANITIZE void addTask_cls(double time, void* addresstofn, double arg, void* addresstocls) {
  mimium::Scheduler& sch = global_runtime->getAudioDriver()->getScheduler();
  sch.addTask(time, addresstofn, arg, addresstocls);
}
double mimium_getnow() {
  return (double)global_runtime->getAudioDriver()->getScheduler().getTime();
}

// TODO(tomoya) ideally we need to move this to base runtime library
void* mimium_malloc(size_t size) {
  void* address = malloc(size);
  global_runtime->push_malloc(address, size);
  return address;
}
}

namespace mimium {
Runtime_LLVM::Runtime_LLVM(std::string const& filename_i, std::shared_ptr<AudioDriver> a,
                           bool isjit)
    : Runtime(filename_i, std::move(a)) {
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();
  jitengine = std::make_unique<llvm::orc::MimiumJIT>();
}

void Runtime_LLVM::executeModule(std::unique_ptr<llvm::Module> module) {
  llvm::Error err = jitengine->addModule(std::move(module));
  Logger::debug_log(err, Logger::ERROR);
  auto mainfun = jitengine->lookup("mimium_main");

  Logger::debug_log(mainfun, Logger::ERROR);
  auto mimium_main_function = llvm::jitTargetAddressToPointer<void* (*)()>(mainfun->getAddress());
  //
  mimium_main_function();
  //
  if (auto symbolorerror = jitengine->lookup("dsp")) {
    auto address = (DspFnType)symbolorerror->getAddress();
    hasdsp = true;
  } else {
    auto err = symbolorerror.takeError();
    hasdsp = false;
    Logger::debug_log("dsp function not found", Logger::INFO);
    llvm::consumeError(std::move(err));
  }
}
void Runtime_LLVM::start() {
  auto& sch = audiodriver->getScheduler();
  if (hasdsp || sch.hasTask()) {
    audiodriver->start();
    {
      auto& waitc = sch.getWaitController();
      std::unique_lock<std::mutex> uniq_lk(waitc.mtx);
      // aynchronously wait until scheduler stops
      waitc.cv.wait(uniq_lk, [&]() { return waitc.isready; });
    }
  }
}

}  // namespace mimium