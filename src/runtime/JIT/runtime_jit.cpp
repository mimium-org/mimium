/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "runtime/JIT/runtime_jit.hpp"
#include <llvm/IRReader/IRReader.h>
#include "runtime/JIT/jit_engine.hpp"

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
  runtime->push_malloc(address, size);
  return address;
}
}

namespace mimium {
Runtime_LLVM::Runtime_LLVM(std::unique_ptr<llvm::LLVMContext> ctx,
                           std::unique_ptr<llvm::Module> module, std::string const& /*filename_i*/,
                           std::unique_ptr<AudioDriver> a, bool optimize)
    : Runtime(std::move(a)), module(std::move(module)) {
  init(std::move(ctx), optimize);
}

Runtime_LLVM::Runtime_LLVM(std::string const& filepath, std::unique_ptr<AudioDriver> a,
                           bool optimize)
    : Runtime(std::move(a)) {
  auto ctx = std::make_unique<llvm::LLVMContext>();
  llvm::SMDiagnostic errorreporter;
  module = llvm::parseIRFile(filepath, errorreporter, *ctx);
  init(std::move(ctx), optimize);
}

Runtime_LLVM::~Runtime_LLVM() = default;

llvm::LLVMContext& Runtime_LLVM::getLLVMContext() { return jitengine->getContext(); }

void Runtime_LLVM::init(std::unique_ptr<llvm::LLVMContext> ctx, bool optimize) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
  llvm::InitializeNativeTargetDisassembler();
  using optlevel = llvm::orc::MimiumJIT::OptimizeLevel;
  auto opt = optimize ? optlevel::NORMAL : optlevel::NO;
  jitengine = std::make_unique<llvm::orc::MimiumJIT>(std::move(ctx), opt);
}
void Runtime_LLVM::runMainFun() {
  assert(module != nullptr);
  llvm::Error err = jitengine->addModule(std::move(this->module));
  if (err) { llvm::errs() << err << "\n"; };
  auto mainfun = jitengine->lookup("mimium_main");

  if (!mainfun) { llvm::errs() << mainfun.takeError() << "\n"; }

  auto mimium_main_function =
      llvm::jitTargetAddressToPointer<void* (*)(void*)>(mainfun->getAddress());
  //
  mimium_main_function(static_cast<Runtime*>(this));
  //
  auto symbol_or_error = jitengine->lookup("dsp");
  if (symbol_or_error) {
    hasdsp = true;
  } else {
    auto err = symbol_or_error.takeError();
    hasdsp = false;
    Logger::debug_log("dsp function not found", Logger::INFO);
    llvm::consumeError(std::move(err));
  }
}
void Runtime_LLVM::start() {
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

}  // namespace mimium