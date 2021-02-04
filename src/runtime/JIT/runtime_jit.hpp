/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "runtime/backend/audiodriver.hpp"

namespace llvm {
class LLVMContext;
class Module;
namespace orc {
class MimiumJIT;
}
}  // namespace llvm

namespace mimium {

class MIMIUM_DLL_PUBLIC Runtime_LLVM : public Runtime,
                                       public std::enable_shared_from_this<Runtime_LLVM> {
 public:
  explicit Runtime_LLVM(std::unique_ptr<llvm::LLVMContext> ctx, std::unique_ptr<llvm::Module>,
                        std::string const& filename = "untitled.mmm",
                        std::unique_ptr<AudioDriver> a = nullptr, bool optimize = true);
  explicit Runtime_LLVM(std::string const& filepath, std::unique_ptr<AudioDriver> a = nullptr,
                        bool optimize = true);
  ~Runtime_LLVM() override;
  void start() override;
  void runMainFun() override;

  auto& getJitEngine() { return *jitengine; }
  llvm::LLVMContext& getLLVMContext();

 private:
  // called by constructor.
  void init(std::unique_ptr<llvm::LLVMContext> ctx, bool optimize);
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::orc::MimiumJIT> jitengine;
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