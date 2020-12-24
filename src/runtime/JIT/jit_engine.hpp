/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */



#pragma once
#include <iostream>
#include <memory>

#include "llvm-c/ExecutionEngine.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/Layer.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/Error.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Vectorize.h"

#include "basic/helper_functions.hpp" //load NO_SANITIZE

#define LAZY_ENABLE 0
#if LAZY_ENABLE
#define LLJITCLASS LLLazyJIT
#else
#define LLJITCLASS LLJIT
#endif

namespace llvm {
namespace orc {

class MimiumJIT {
 private:
#if LAZY_ENABLE
  std::unique_ptr<LLJITCLASS> lllazyjit;
#else
  std::unique_ptr<LLJIT> lllazyjit;
#endif
  ExecutionSession& ES;
  const DataLayout& DL;
  JITDylib& MainJD;

  MangleAndInterner Mangle;
  ThreadSafeContext Ctx;

 public:
  MimiumJIT(std::unique_ptr<LLVMContext> ctx)
      : lllazyjit(createEngine()),
        ES(lllazyjit->getExecutionSession()),
        DL(lllazyjit->getDataLayout()),
        MainJD(lllazyjit->getMainJITDylib()),
        Mangle(ES, this->DL),
        Ctx(std::move(ctx)) {
#if LAZY_ENABLE
    lllazyjit->setLazyCompileTransform(optimizeModule);
#endif
// MainJD.getExecutionSession()
#if LLVM_VERSION_MAJOR >= 10
    MainJD.addGenerator(
        cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(DL.getGlobalPrefix())));
#else
    MainJD.setGenerator(
        cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(DL.getGlobalPrefix())));
#endif
  }
//Creates LLJIT engine. Note that builder.create causes container overflow inside llvm library.
// maybe in llvm::LLVMTargetMachine::initAsmInfo()?

NO_SANITIZE static std::unique_ptr<LLJITCLASS> createEngine() {
#if LAZY_ENABLE
    auto builder = LLLazyJITBuilder();
#else
    auto builder = LLJITBuilder();
#endif
    auto jit = builder.create();
    llvm::logAllUnhandledErrors(jit.takeError(), llvm::errs());
    return std::move(jit.get());
  }
  Error addModule(std::unique_ptr<Module> M) {
#if LAZY_ENABLE
    return lllazyjit->addLazyIRModule(ThreadSafeModule(std::move(M), Ctx));
#else
    return lllazyjit->addIRModule(ThreadSafeModule(std::move(M), Ctx));
#endif
  }
  Expected<JITEvaluatedSymbol> lookup(StringRef name) { return lllazyjit->lookup(name); }

  Error addSymbol(StringRef name, void* ptr) {
    // auto symbol = JITEvaluatedSymbol(pointerToJITTargetAddress(&puts),
    // JITSymbolFlags::Absolute); auto res =    MainJD.define(absoluteSymbols({{
    // Mangle("puts"), symbol}}));
    // symbol.setFlags(JITSymbolFlags::FlagNames::Callable);
    // auto res = lllazyjit->defineAbsolute(name, symbol);
    auto res = lllazyjit->lookup("addTask");
    ES.dump(errs());

    // auto address =
    // jitTargetAddressToPointer<int(*)(char*)>(test->getAddress());
    MainJD.dump(errs());
    return res.takeError();
  }

  static Expected<ThreadSafeModule> optimizeModule(ThreadSafeModule M,
                                                   const MaterializationResponsibility& R) {
// Create a function pass manager.
#if LLVM_VERSION_MAJOR >= 10
    auto FPM = std::make_unique<legacy::FunctionPassManager>(M.getModuleUnlocked());
#else
    auto FPM = std::make_unique<legacy::FunctionPassManager>(M.getModule());
#endif
    // Add some optimizations.
    // FPM->add(createPromoteMemoryToRegisterPass());//mem2reg
    // FPM->add(createDeadStoreEliminationPass());
    // FPM->add(createInstructionCombiningPass());
    // FPM->add(createReassociatePass());
    // FPM->add(createGVNPass());
    // FPM->add(createCFGSimplificationPass());
    // FPM->add(createLoopVectorizePass());
    FPM->doInitialization();

// Run the optimizations over all functions in the module being added to
// the JIT.
#if LLVM_VERSION_MAJOR >= 10
    M.withModuleDo(
        [&](Module& m) { std::for_each(m.begin(), m.end(), [&](auto& f) { FPM->run(f); }); });
#else
    for (auto& f : M.getModule()->functions()) { FPM->run(f); }
#endif
    return M;
  }
  [[nodiscard]] const DataLayout& getDataLayout() const { return DL; }
  LLVMContext& getContext() { return *Ctx.getContext(); }
};
}  // namespace orc
}  // namespace llvm
