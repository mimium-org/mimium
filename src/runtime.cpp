#include "runtime.hpp"

#include <memory>
#include <stdexcept>

#include "closure_convert.hpp"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvmgenerator.hpp"

namespace mimium {

Runtime_LLVM::Runtime_LLVM(std::string filename_i, bool isjit)
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
  llvmgenerator = std::make_shared<LLVMGenerator>(filename_i, isjit);
  closureconverter = std::make_shared<ClosureConverter>(typevisitor.getEnv());
}  // temporary,jit is off

void Runtime_LLVM::addScheduler(bool issoundfile) {
  if (issoundfile) {
    sch = std::make_shared<SchedulerSndFile>(shared_from_this(),waitc);
  } else {
    sch = std::make_shared<SchedulerRT>(shared_from_this(),waitc);
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
int Runtime_LLVM::execute() {
  auto& jit = llvmgenerator->getJitEngine();
  if (auto symbolorerror = jit.lookup("dac")) {
    auto address = (double (*)(double))symbolorerror->getAddress();
    dspfn_address = address;
    hasdsp = true;
  } else {
    auto err = symbolorerror.takeError();
    Logger::debug_log("dsp function not found", Logger::WARNING);
    llvm::consumeError(std::move(err));
    dspfn_address = nullptr;
    hasdsp = false;
  }
  return llvmgenerator->execute();
}
void Runtime_LLVM::start() {
  sch->start();
  {
    std::unique_lock<std::mutex> uniq_lk(waitc.mtx);
    // aynchronously wait until scheduler stops
    waitc.cv.wait(uniq_lk, [&]() { return waitc.isready; });
  }
  sch->stopAudioDriver();
}

void Runtime_LLVM::executeTask(const LLVMTaskType& task) {
  auto& [addresstofn, arg, ptrtotarget] = task;
  auto& jit = llvmgenerator->getJitEngine();
  // auto& type = llvmgenerator->getTaskInfoList()[tasktypeid];

  // std::visit(overloaded{
  //   [](recursive_wrapper<types::Function>& f){
  //      auto ff= (types::Function)(f);
  //      if(auto rettype = std::get_if<types::Float>(&ff.getReturnType())){

  //      }
  //   },
  //   [](auto& other){ throw std::runtime_error("invalid task type");}
  // },type);
  auto fn = reinterpret_cast<double (*)(double)>(addresstofn);

  double res = fn(arg);
  *ptrtotarget = res;
}

dtodtype Runtime_LLVM::getDspFn() { return dspfn_address; }

}  // namespace mimium