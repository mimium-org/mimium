#pragma once
#include <iostream>
#include <memory>

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
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/Error.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

namespace llvm {
namespace orc {

class MimiumJIT {
 private:
  std::unique_ptr<LLLazyJIT> lllazyjit;
  ExecutionSession& ES;
  const DataLayout& DL;
  JITDylib& MainJD;

  MangleAndInterner Mangle;
  ThreadSafeContext Ctx;

 public:
  MimiumJIT()
      : lllazyjit(cantFail(createEngine())),
        ES(lllazyjit->getExecutionSession()),
        DL(lllazyjit->getDataLayout()),
        MainJD(lllazyjit->getMainJITDylib()),
        Mangle(ES, this->DL),
        Ctx(std::make_unique<LLVMContext>()) {
    lllazyjit->setLazyCompileTransform(optimizeModule);
    MainJD.setGenerator(
        cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(
            DL.getGlobalPrefix())));
  }
  static Expected<std::unique_ptr<LLLazyJIT>> createEngine() {
    auto builder = LLLazyJITBuilder();
    auto jit = builder.create();

    return jit;
  }
  Error addModule(std::unique_ptr<Module> M) {
    return lllazyjit->addLazyIRModule(ThreadSafeModule(std::move(M), Ctx));
  }
  Expected<JITEvaluatedSymbol> lookup(StringRef name) {
    return lllazyjit->lookup(name);
  }

  Error addSymbol(StringRef name, void* ptr) {
    auto symbol =
        JITEvaluatedSymbol(pointerToJITTargetAddress(ptr), JITSymbolFlags());
    return lllazyjit->defineAbsolute(name, symbol);
  }

  static Expected<ThreadSafeModule> optimizeModule(
      ThreadSafeModule M, const MaterializationResponsibility& R) {
    // Create a function pass manager.

    auto FPM = std::make_unique<legacy::FunctionPassManager>(M.getModule());

    // Add some optimizations.
    FPM->add(createInstructionCombiningPass());
    FPM->add(createReassociatePass());
    FPM->add(createGVNPass());
    FPM->add(createCFGSimplificationPass());
    FPM->doInitialization();

    // Run the optimizations over all functions in the module being added to
    // the JIT.
    for (auto& fun : *M.getModule()) {
      FPM->run(fun);
    }
    return M;
  }
  [[nodiscard]] const DataLayout& getDataLayout() const { return DL; }
  LLVMContext& getContext() { return *Ctx.getContext(); }
};
}  // namespace orc
}  // namespace llvm
