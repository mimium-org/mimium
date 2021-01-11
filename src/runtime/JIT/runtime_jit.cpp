/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "runtime/JIT/runtime_jit.hpp"

extern "C" {
void setDspParams(void* runtimeptr, void* dspfn, void* clsaddress, void* memobjaddress) {
  auto* runtime = reinterpret_cast<mimium::Runtime*>(runtimeptr);
  auto audiodriver = runtime->getAudioDriver();
  audiodriver->setDspFnInfos(
      mimium::DspFnInfos{reinterpret_cast<mimium::DspFnPtr>(dspfn), clsaddress, memobjaddress});
}

NO_SANITIZE void addTask(void* runtimeptr, double time, void* addresstofn, double arg) {
  auto* runtime = reinterpret_cast<mimium::Runtime*>(runtimeptr);
  mimium::Scheduler& sch = runtime->getAudioDriver()->getScheduler();
  sch.addTask(time, addresstofn, arg, nullptr);
}
NO_SANITIZE void addTask_cls(void* runtimeptr, double time, void* addresstofn, double arg,
                             void* addresstocls) {
  auto* runtime = reinterpret_cast<mimium::Runtime*>(runtimeptr);
  mimium::Scheduler& sch = runtime->getAudioDriver()->getScheduler();
  sch.addTask(time, addresstofn, arg, addresstocls);
}
double mimium_getnow(void* runtimeptr) {
  auto* runtime = reinterpret_cast<mimium::Runtime*>(runtimeptr);
  return (double)runtime->getAudioDriver()->getScheduler().getTime();
}

// TODO(tomoya) ideally we need to move this to base runtime library
void* mimium_malloc(void* runtimeptr, size_t size) {
  auto* runtime = reinterpret_cast<mimium::Runtime*>(runtimeptr);
  void* address = malloc(size);
  runtime->push_malloc(address, size);
  return address;
}
}

namespace mimium {
Runtime_LLVM::Runtime_LLVM(std::unique_ptr<llvm::LLVMContext> ctx, std::string const& filename_i,
                           std::shared_ptr<AudioDriver> a, bool isjit)
    : Runtime(filename_i, std::move(a)) {
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();
  jitengine = std::make_unique<llvm::orc::MimiumJIT>(std::move(ctx));
}

void Runtime_LLVM::executeModule(std::unique_ptr<llvm::Module> module) {
  llvm::Error err = jitengine->addModule(std::move(module));
  if (err) { llvm::errs() << err << "\n"; };
  auto mainfun = jitengine->lookup("mimium_main");

  if (!mainfun) { llvm::errs() << mainfun.takeError() << "\n"; }

  auto mimium_main_function = llvm::jitTargetAddressToPointer<void* (*)(void*)>(mainfun->getAddress());
  //
  mimium_main_function(this);
  //
  if (auto symbolorerror = jitengine->lookup("dsp")) {
    auto address = (DspFnPtr)symbolorerror->getAddress();
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