/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <memory>
#include <string>
#include "runtime/executionengine/executionengine.hpp"

namespace llvm {
class LLVMContext;
class Module;
namespace orc {
class MimiumJIT;
}
}  // namespace llvm

namespace mimium {

class MIMIUM_DLL_PUBLIC LLVMJitExecutionEngine : public ExecutionEngine {
 public:
  explicit LLVMJitExecutionEngine(std::unique_ptr<llvm::LLVMContext> ctx,
                                  std::unique_ptr<llvm::Module>,
                                  std::string const& filename = "untitled.mmm",
                                  bool optimize = true);
  explicit LLVMJitExecutionEngine(std::string const& filepath, bool optimize = true);
  ~LLVMJitExecutionEngine() override;
  bool runMainFunction(Runtime* runtime_ptr) override;

 private:
  // called by constructor.
  void initInternal(std::unique_ptr<llvm::LLVMContext> ctx, bool optimize);
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::orc::MimiumJIT> jitengine;
};

}  // namespace mimium