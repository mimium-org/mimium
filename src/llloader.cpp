/**
 * A simple expression parser and evaluator. Evaluates user-input.
 * Supports +, -, *, /, ^ and grouping with parenthesis.
 * Correctly handles precedence associativity.
 */
#include <unistd.h>

#include <csignal>
#include <fstream>
#include <string>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/SourceMgr.h>

#include "basic/helper_functions.hpp"
// #include "cli_tools.cpp"
#include "llvm/Support/CommandLine.h"
namespace cl = llvm::cl;
using Logger = mimium::Logger;
// #include "compiler/compiler.hpp"
#include "runtime/runtime.hpp"

extern "C" {

mimium::Scheduler* global_sch;
void addTask(double time, void* addresstofn,  double arg) {
  global_sch->addTask(time, addresstofn, arg, nullptr);
}
void addTask_cls(double time, void* addresstofn,  double arg,
             void* addresstocls) {
  global_sch->addTask(time, addresstofn, arg, addresstocls);
}
int myprint(double d) {
  std::cerr << d << std::endl;
  return 0;
}
// double asyncTaskCaller(double d,double(*fnptr)(double)){
//  auto future = std::async(std::launch::deferred,fnptr,d);
//  global_sch->addTask(time,future);//executeTask()でfuture.getする
//  future.wait();
//   return future.get();
// }

}

std::function<void(int)> shutdown_handler;
void signalHandler(int signo) { shutdown_handler(signo); }

auto main(int argc, char** argv) -> int {
    int returncode = 0;
  enum class CompileStage : int {
    AST = 0,
    AST_UNIQUENAME,
    TYPEINFOS,
    MIR,
    MIR_CC,
    LLVMIR,
    EXECUTE
  };
  cl::OptionCategory general_category("General Options", "");
  cl::opt<std::string> input_filename(cl::Positional, cl::desc("<input file>"),
                                      cl::init("-"), cl::cat(general_category));
     
  cl::ResetAllOptionOccurrences();

  cl::HideUnrelatedOptions(general_category);
  cl::ParseCommandLineOptions(argc, argv, "Mimium\n");  // launch cli helper

  std::ifstream input(input_filename.c_str());
  signal(SIGINT, signalHandler);
  Logger::current_report_level = Logger::INFO;
    auto runtime = std::make_shared<mimium::Runtime_LLVM>();
//   auto compiler = std::make_unique<mimium::Compiler>(runtime->getLLVMContext());
  shutdown_handler = [&runtime](int /*signal*/) {
    if (runtime->isrunning()) {
      runtime->stop();
    }
    std::cerr << "Interuppted by key" << std::endl;
    exit(0);
  };
  runtime->addScheduler(false);
  global_sch = runtime->getScheduler().get();

llvm::SMDiagnostic errorreporter;
  if (!input.good()) {  
    Logger::debug_log("Specify file name, repl mode is not implemented yet",Logger::ERROR);
// filename is empty:enter repl mode
  } else {  // try to parse and exec input file
    try {
      std::string filename = input_filename.c_str();
                    Logger::debug_log("Opening " + filename, Logger::INFO);

    auto m = llvm::parseIRFile(filename, errorreporter, runtime->getLLVMContext()) ;
        runtime->executeModule(std::move(m));
        runtime->start();//start() blocks thread until scheduler stops
        returncode = 0;


    } catch (std::exception& e) {
      mimium::Logger::debug_log(e.what(), mimium::Logger::ERROR);
      runtime->stop();
      returncode=1;
    }
  } 
  llvm::errs() << "return code: " << returncode << "\n";
  return returncode;
}
