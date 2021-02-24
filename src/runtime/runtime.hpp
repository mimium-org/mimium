/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <list>
#include "export.hpp"

#include "basic/helper_functions.hpp"
#include "runtime/runtime_defs.hpp"
#include "runtime/scheduler.hpp"

namespace mimium {
class AudioDriver;
class ExecutionEngine;
class MIMIUM_DLL_PUBLIC Runtime {
 public:
  explicit Runtime(std::unique_ptr<AudioDriver> a, std::unique_ptr<ExecutionEngine> e);

  virtual ~Runtime() {
    for (auto&& [address, size] : malloc_container) { free(address); }
  };

  virtual void runMainFun();
  virtual void start();
  AudioDriver& getAudioDriver();
  [[nodiscard]] bool hasDsp() const { return hasdsp; }
  [[nodiscard]] bool hasDspCls() const { return hasdspcls; }
  void pushMalloc(void* address, size_t size);

 protected:
  std::unique_ptr<AudioDriver> audiodriver;
  std::unique_ptr<ExecutionEngine> executionengine;
  bool hasdsp = false;
  bool hasdspcls = false;
  std::list<std::pair<void*, size_t>> malloc_container{};
};

extern "C" {
MIMIUM_DLL_PUBLIC void setDspParams(void* runtimeptr, void* dspfn, void* clsaddress,
                                    void* memobjaddress, int in_numchs, int out_numchs);
MIMIUM_DLL_PUBLIC void addTask(void* runtimeptr, double time, void* addresstofn, double arg);
MIMIUM_DLL_PUBLIC void addTask_cls(void* runtimeptr, double time, void* addresstofn, double arg,
                                   void* addresstocls);
MIMIUM_DLL_PUBLIC double mimium_getnow(void* runtimeptr);
MIMIUM_DLL_PUBLIC void* mimium_malloc(void* runtimeptr, size_t size);
}

}  // namespace mimium