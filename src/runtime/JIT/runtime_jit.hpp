/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "runtime/JIT/jit_engine.hpp"
#include "runtime/runtime.hpp"
#include "runtime/scheduler/scheduler.hpp"

namespace mimium {

class Runtime_LLVM : public Runtime<TaskType>, public std::enable_shared_from_this<Runtime_LLVM> {
 public:
  explicit Runtime_LLVM(std::string filename = "untitled.mmm", bool isjit = true);

  ~Runtime_LLVM() = default;
  void addScheduler() override;
  void start() override;
  DspFnType getDspFn() override;
  void* getDspFnCls() override;

  void executeModule(std::unique_ptr<llvm::Module> module);
  auto& getJitEngine() { return *jitengine; }
  llvm::LLVMContext& getLLVMContext() { return jitengine->getContext(); }
  void addAudioDriver(std::shared_ptr<AudioDriver> a) override;

 private:
  DspFnType dspfn_address = nullptr;
  void* dspfn_cls_address = nullptr;
  std::unique_ptr<llvm::orc::MimiumJIT> jitengine;
};
}  // namespace mimium