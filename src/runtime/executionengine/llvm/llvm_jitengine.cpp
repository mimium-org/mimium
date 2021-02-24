/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "llvm_jitengine.hpp"
#include <llvm/IRReader/IRReader.h>
#include "basic/error_def.hpp"
#include "mimium_llvm_orcjit.hpp"
namespace mimium {
LLVMJitExecutionEngine::LLVMJitExecutionEngine(std::unique_ptr<llvm::LLVMContext> ctx,
                                               std::unique_ptr<llvm::Module> module,
                                               std::string const& /*filename_i*/, bool optimize)
    : ExecutionEngine(), module(std::move(module)) {
  initInternal(std::move(ctx), optimize);
}

LLVMJitExecutionEngine::LLVMJitExecutionEngine(std::string const& filepath, bool optimize)
    : ExecutionEngine(), module() {
  auto ctx = std::make_unique<llvm::LLVMContext>();
  llvm::SMDiagnostic errorreporter;
  module = llvm::parseIRFile(filepath, errorreporter, *ctx);
  initInternal(std::move(ctx), optimize);
}
LLVMJitExecutionEngine::~LLVMJitExecutionEngine() = default;

void LLVMJitExecutionEngine::initInternal(std::unique_ptr<llvm::LLVMContext> ctx, bool optimize) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
  llvm::InitializeNativeTargetDisassembler();
  using optlevel = llvm::orc::MimiumJIT::OptimizeLevel;
  auto opt = optimize ? optlevel::NORMAL : optlevel::NO;
  jitengine = std::make_unique<llvm::orc::MimiumJIT>(std::move(ctx), opt);
}
bool LLVMJitExecutionEngine::runMainFunction(Runtime* runtime_ptr) {
  assert(module != nullptr);
  llvm::Error err = jitengine->addModule(std::move(this->module));
  if (err) { llvm::errs() << err << "\n"; };
  auto mainfun = jitengine->lookup("mimium_main");

  if (!mainfun) {
    std::string tmpout;
    llvm::raw_string_ostream oss(tmpout);
    oss << mainfun.takeError();
    throw mimium::RuntimeError(oss.str());
  }

  auto mimium_main_function =
      llvm::jitTargetAddressToPointer<void* (*)(void*)>(mainfun->getAddress());
  //
  mimium_main_function(runtime_ptr);
  //
  auto symbol_or_error = jitengine->lookup("dsp");
  if (!symbol_or_error) {
    auto dsperr = symbol_or_error.takeError();
    Logger::debug_log("dsp function not found", Logger::INFO);
    llvm::consumeError(std::move(dsperr));
    return false;
  }
  return true;
}

}  // namespace mimium