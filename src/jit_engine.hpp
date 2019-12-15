#pragma once
#include <memory>
#include <iostream>

#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

namespace llvm {
namespace orc {

class MimiumJIT {
 private:
  ExecutionSession ES;
  RTDyldObjectLinkingLayer ObjectLayer;
  IRCompileLayer CompileLayer;
  IRTransformLayer OptimizeLayer;

  DataLayout DL;
  MangleAndInterner Mangle;
  ThreadSafeContext Ctx;

  JITDylib& MainJD;

 public:
  MimiumJIT(JITTargetMachineBuilder JTMB, DataLayout DL)
      : ObjectLayer(ES,
                    []() { return std::make_unique<SectionMemoryManager>(); }),
        CompileLayer(ES, ObjectLayer, ConcurrentIRCompiler(std::move(JTMB))),
        OptimizeLayer(ES, CompileLayer, optimizeModule),
        DL(DL),
        Mangle(ES, this->DL),
        Ctx(std::make_unique<LLVMContext>()),
        MainJD(ES.getMainJITDylib()) {
            MainJD.setGenerator(
        cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(
            DL.getGlobalPrefix())));
  }
  static Expected<std::unique_ptr<MimiumJIT>> createEngine() {
    auto jtmb = JITTargetMachineBuilder::detectHost();

    if (!jtmb) {
      return jtmb.takeError();
    }

    auto dl = jtmb->getDefaultDataLayoutForTarget();
    if (!dl) {
      return dl.takeError();
    }
    return std::make_unique<MimiumJIT>(std::move(*jtmb), std::move(*dl));
  }
  Error addModule(std::unique_ptr<Module> M) {
    M->dump();
    return OptimizeLayer.add(MainJD, ThreadSafeModule(std::move(M), Ctx));
  }
  Expected<JITEvaluatedSymbol> lookup(StringRef name) {
    MainJD.dump(llvm::errs());
    ES.dump(llvm::errs());
    return ES.lookup({&ES.getMainJITDylib()}, name.str());

  }
  static Expected<ThreadSafeModule> optimizeModule(
      ThreadSafeModule M, const MaterializationResponsibility& R) {
    // Create a function pass manager.

    auto FPM = std::make_unique<legacy::FunctionPassManager>(M.getModule());

    // Add some optimizations.
    // FPM->add(createInstructionCombiningPass());
    // FPM->add(createReassociatePass());
    // FPM->add(createGVNPass());
    // FPM->add(createCFGSimplificationPass());
    FPM->doInitialization();

    // Run the optimizations over all functions in the module being added to
    // the JIT.
    for (auto& fun : *M.getModule()) {
      FPM->run(fun);
    }
    M.getModule()->dump();
    return M;
  }
  const DataLayout& getDataLayout() const { return DL; }
  LLVMContext& getContext() { return *Ctx.getContext(); }
};
}  // namespace orc
}  // namespace llvm
