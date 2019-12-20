#include "runtime.hpp"
#include <memory>
#include "closure_convert.hpp"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvmgenerator.hpp"


namespace mimium {



Runtime_LLVM::Runtime_LLVM(std::string filename_i,bool isjit)
    : Runtime<LLVMTaskType>(filename_i),
      alphavisitor(),
      typevisitor(),
      ti_ptr(&typevisitor),
      knormvisitor(ti_ptr),
      closureconverter(),
      llvmgenerator() {
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();
        llvmgenerator = std::make_shared<LLVMGenerator>(filename_i,isjit);
        closureconverter = std::make_shared<ClosureConverter>(typevisitor.getEnv());
      } //temporary,jit is off

void Runtime_LLVM::addScheduler(bool issoundfile){
    if (issoundfile) {
    sch = std::make_shared<SchedulerSndFile>(shared_from_this());
  } else {
    sch = std::make_shared<SchedulerRT>(shared_from_this());
  }
  auto& jit = llvmgenerator->getJitEngine();
  auto addtaskaddress = &Scheduler<LLVMTaskType>::addTask;
  auto mysize = sizeof(addtaskaddress);
  auto* ptr = reinterpret_cast<char*>(sch.get());
  auto*newptr = static_cast<void*>(ptr + mysize);
  auto err = jit.addSymbol("addTask", newptr);


  if(bool(err)){
    llvm::errs()<<err <<"\n";
  }
}


AST_Ptr Runtime_LLVM::loadSourceFile(std::string filename) {
  this->filename = filename;
  llvmgenerator->reset(filename);
  driver.parsefile(filename);
  return driver.getMainAst();
}
AST_Ptr Runtime_LLVM::alphaConvert(AST_Ptr _ast) {
  _ast->accept(alphavisitor);
  return alphavisitor.getResult();
}
TypeEnv& Runtime_LLVM::typeInfer(AST_Ptr _ast) {
  _ast->accept(typevisitor);
  return typevisitor.getEnv();
}
std::shared_ptr<MIRblock> Runtime_LLVM::kNormalize(AST_Ptr _ast) {
  _ast->accept(knormvisitor);
  return knormvisitor.getResult();
}
std::shared_ptr<MIRblock> Runtime_LLVM::closureConvert(
    std::shared_ptr<MIRblock> mir) {
  return closureconverter->convert(mir);
}
auto Runtime_LLVM::llvmGenarate(std::shared_ptr<MIRblock> mir) -> std::string {
  llvmgenerator->generateCode(mir);
  std::string s;
  llvm::raw_string_ostream ss(s);
  llvmgenerator->outputToStream(ss);
  return s;
}
int Runtime_LLVM::execute(){
  return llvmgenerator->execute();
}

void Runtime_LLVM::executeTask(const LLVMTaskType& task){
  auto& [addresstofn,arg,ptrtotarget] = task;
    auto& jit = llvmgenerator->getJitEngine();

  auto fn = addresstofn;
  // auto res = jit.lookup(fname);
  // Logger::debug_log(res,Logger::ERROR);
  // auto& symbol = res.get();

  // auto fnptr = llvm::jitTargetAddressToPointer<void*>(symbol.getAddress());
  double res  = fn(arg);
  *ptrtotarget = res;//overwrite value....?
  }


}  // namespace mimium