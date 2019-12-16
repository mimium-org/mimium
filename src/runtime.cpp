#include "runtime.hpp"
#include "closure_convert.hpp"
#include "llvmgenerator.hpp"


namespace mimium {

void Runtime::addScheduler(bool issoundfile = false) {
  if (issoundfile) {
    sch = std::make_shared<SchedulerSndFile>(shared_from_this());
  } else {
    sch = std::make_shared<SchedulerRT>(shared_from_this());
  }
}

void Runtime::init() { /* midi.init(); */ }

void Runtime::clear() { clearDriver(); }

void Runtime::start() {
  sch->start();
  running_status = true;
}

void Runtime::stop() {
  sch->stop();
  running_status = false;
}

AST_Ptr Runtime::loadSource(const std::string src) {
  driver.parsestring(src);
  return driver.getMainAst();
}
AST_Ptr Runtime::loadSourceFile(const std::string filename) {
  driver.parsefile(filename);
  return driver.getMainAst();
}

Runtime_LLVM::Runtime_LLVM(std::string filename_i,bool isjit)
    : Runtime(filename_i),
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
  // std::function<void(int,AST_Ptr)> addtaskfn = [&](int time,AST_Ptr task){ sch->addTask(time, task);};
  // auto ptr = reinterpret_cast<void*>( getAddressfromFun(std::move(addtaskfn)));
  // auto err = jit.addSymbol("addTask", ptr);
  // if(bool(err)){
  //   llvm::errs()<<err <<"\n";
  // }
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

}  // namespace mimium