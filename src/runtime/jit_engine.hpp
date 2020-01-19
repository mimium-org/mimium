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
#include "llvm/Transforms/Vectorize.h"
#include "llvm/Transforms/Utils.h"


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
      : lllazyjit(createEngine()),
        ES(lllazyjit->getExecutionSession()),
        DL(lllazyjit->getDataLayout()),
        MainJD(lllazyjit->getMainJITDylib()),
        Mangle(ES, this->DL),
        Ctx(std::make_unique<LLVMContext>()) {
    lllazyjit->setLazyCompileTransform(optimizeModule);
    // MainJD.getExecutionSession()
    MainJD.addGenerator(
        cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(
            DL.getGlobalPrefix())));
  }
  static std::unique_ptr<LLLazyJIT> createEngine() {
    auto builder = LLLazyJITBuilder();
    auto jit = builder.create();
    llvm::logAllUnhandledErrors(jit.takeError(),llvm::errs());
    return std::move(jit.get());
  }
  Error addModule(std::unique_ptr<Module> M) {
    return lllazyjit->addLazyIRModule(ThreadSafeModule(std::move(M), Ctx));
  }
  Expected<JITEvaluatedSymbol> lookup(StringRef name) {
    return lllazyjit->lookup(name);
  }

  Error addSymbol(StringRef name, void* ptr) {
    // auto symbol = JITEvaluatedSymbol(pointerToJITTargetAddress(&puts), JITSymbolFlags::Absolute);
    // auto res =    MainJD.define(absoluteSymbols({{ Mangle("puts"), symbol}}));
    // symbol.setFlags(JITSymbolFlags::FlagNames::Callable);
    // auto res = lllazyjit->defineAbsolute(name, symbol);
    auto res = lllazyjit->lookup("addTask");
    ES.dump(errs());


    // auto address = jitTargetAddressToPointer<int(*)(char*)>(test->getAddress());
    MainJD.dump(errs());
    return res.takeError();
  }

  static Expected<ThreadSafeModule> optimizeModule(
      ThreadSafeModule M, const MaterializationResponsibility& R) {
      // Create a function pass manager.

      auto FPM = std::make_unique<legacy::FunctionPassManager>(M.getModuleUnlocked());

      // Add some optimizations.
      // FPM->add(createPromoteMemoryToRegisterPass());//mem2reg
      FPM->add(createDeadStoreEliminationPass());
      FPM->add(createInstructionCombiningPass());
      FPM->add(createReassociatePass());
      FPM->add(createGVNPass());
      FPM->add(createCFGSimplificationPass());
      FPM->add(createLoopVectorizePass());
      FPM->doInitialization();

      // Run the optimizations over all functions in the module being added to
      // the JIT.
      M.withModuleDo([&](Module& m){
        std::for_each(m.begin(), m.end(), [&](auto& f){FPM->run(f);});
      });
      return M;
  }
  [[nodiscard]] const DataLayout& getDataLayout() const {
      return DL; }
  LLVMContext& getContext() {
      return *Ctx.getContext(); }
  };
}  // namespace orc
}  // namespace orc
