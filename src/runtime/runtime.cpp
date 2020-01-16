#include "runtime/runtime.hpp"

#include <memory>
#include <stdexcept>

#include "compiler/closure_convert.hpp"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "compiler/llvmgenerator.hpp"

namespace mimium {

Runtime_LLVM::Runtime_LLVM(std::string filename_i, bool isjit)
    : Runtime<LLVMTaskType>(filename_i) {
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();
  jitengine = std::make_unique<llvm::orc::MimiumJIT>();
}

void Runtime_LLVM::addScheduler(bool issoundfile) {
  if (issoundfile) {
    sch = std::make_shared<SchedulerSndFile>(shared_from_this(), waitc);
  } else {
    sch = std::make_shared<SchedulerRT>(shared_from_this(), waitc);
  }
}



void Runtime_LLVM::executeModule(std::unique_ptr<llvm::Module> module) {
  llvm::Error err = jitengine->addModule(std::move(module));
  Logger::debug_log(err, Logger::ERROR);
  auto mainfun = jitengine->lookup("mimium_main");
  Logger::debug_log(mainfun, Logger::ERROR);
  auto fnptr =
      llvm::jitTargetAddressToPointer<void* (*)()>(mainfun->getAddress());
  //
  auto dsp_cls_address = fnptr();
  //
  if (auto symbolorerror = jitengine->lookup("dsp")) {
    auto address = (DspFnType)symbolorerror->getAddress();
    dspfn_address = address;
    hasdsp = true;
    hasdspcls = dsp_cls_address != nullptr;
    if (hasdspcls) {
      this->dspfn_cls_address = dsp_cls_address;
    }

  } else {
    auto err = symbolorerror.takeError();
    Logger::debug_log("dsp function not found", Logger::INFO);
    llvm::consumeError(std::move(err));
    dspfn_address = nullptr;
    hasdsp = false;
  }
}
// run audio driver and scheduler if theres some task, dsp function, or both.
void Runtime_LLVM::start() {
  if (hasdsp || sch->hasTask()) {
    sch->start();
    {
      std::unique_lock<std::mutex> uniq_lk(waitc.mtx);
      // aynchronously wait until scheduler stops
      waitc.cv.wait(uniq_lk, [&]() { return waitc.isready; });
    }
    sch->stopAudioDriver();
  }
}

void Runtime_LLVM::executeTask(const LLVMTaskType& task) {
  auto& [addresstofn, arg, addresstocls] = task;

  if (addresstocls == nullptr) {
    auto fn = reinterpret_cast<void (*)(double)>(addresstofn);
    fn(arg);
  } else {
    auto fn = reinterpret_cast<void (*)(double, void*)>(addresstofn);
    fn(arg, addresstocls);
  }
}

DspFnType Runtime_LLVM::getDspFn() { return dspfn_address; }
void* Runtime_LLVM::getDspFnCls() { return dspfn_cls_address; }

}  // namespace mimium