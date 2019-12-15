/**
 * A simple expression parser and evaluator. Evaluates user-input.
 * Supports +, -, *, /, ^ and grouping with parenthesis.
 * Correctly handles precedence associativity.
 */
#include <unistd.h>

#include <csignal>
#include <fstream>
#include <string>

#include "helper_functions.hpp"
// #include "cli_tools.cpp"
#include "llvm/Support/CommandLine.h"
namespace cl = llvm::cl;
using Logger = mimium::Logger;

#include "runtime.hpp"

std::function<void(int)> shutdown_handler;
void signalHandler(int signo) { shutdown_handler(signo); }
auto main(int argc, char** argv) -> int {
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

  cl::opt<CompileStage> compile_stage(
      cl::desc("Printing Debug Infomations"),
      cl::values(
          clEnumValN(CompileStage::AST, "emit-ast",
                     "Enable trivial optimizations"),
          clEnumValN(CompileStage::AST_UNIQUENAME, "emit-ast-u",
                     "Enable default optimizations"),
          clEnumValN(CompileStage::TYPEINFOS, "emit-types",
                     "emit type information for all variables to stdout"),
          clEnumValN(CompileStage::MIR, "emit-mir", "emit MIR to stdout"),
          clEnumValN(CompileStage::MIR_CC, "emit-mir-cc",
                     "emit MIR after closure convertsion to stdout"),

          clEnumValN(CompileStage::LLVMIR, "emit-llvm",
                     "emit LLVM IR to stdout")),
      cl::cat(general_category));
  compile_stage.setInitialValue(CompileStage::EXECUTE);
  cl::opt<bool> snd_file("sndfile",
                         cl::desc("write out a sound file as an output"));
  cl::ResetAllOptionOccurrences();

  cl::HideUnrelatedOptions(general_category);
  cl::ParseCommandLineOptions(argc, argv, "Mimium\n");  // launch cli helper

  std::ifstream input(input_filename.c_str());
  signal(SIGINT, signalHandler);
  Logger::current_report_level = Logger::INFO;
  auto runtime = std::make_shared<mimium::Runtime_LLVM>();
  runtime->init();
  shutdown_handler = [&runtime](int /*signal*/) {
    if (runtime->isrunning()) {
      runtime->stop();
    }
    std::cerr << std::endl << "Interuppted by key" << std::endl;
    exit(0);
  };

  runtime->addScheduler(snd_file);
  if (!input.good()) {  // filename is empty:enter repl mode
    std::string line;
    Logger::debug_log("start", Logger::INFO);
    while (std::getline(std::cin, line)) {
      runtime->clearDriver();
      runtime->loadSource(line);
      // now load source is void function, how to debug print?
      // std::cout << resstr << std::endl;
    }
  } else {  // try to parse and exec input file
    try {
      std::string filename = input_filename.c_str();
      Logger::debug_log("Opening: " + filename, Logger::INFO);

      int stage = 0;
      AST_Ptr ast, ast_u;
      std::shared_ptr<mimium::MIRblock> mir, mir_cc;
      mimium::TypeEnv& typeinfos = runtime->getTypeEnv();
      std::string llvmir;
      int returncode;
      while (stage <= static_cast<int>(compile_stage.getValue())) {
        switch (static_cast<CompileStage>(stage)) {
          case CompileStage::AST:
            ast = runtime->loadSourceFile(filename);
            break;
          case CompileStage::AST_UNIQUENAME:
            ast_u = runtime->alphaConvert(ast);
            break;
          case CompileStage::TYPEINFOS:
            typeinfos = runtime->typeInfer(ast_u);
            break;
          case CompileStage::MIR:
            mir = runtime->kNormalize(ast_u);
            break;
          case CompileStage::MIR_CC:
            mir_cc = runtime->closureConvert(mir);
            break;
          case CompileStage::LLVMIR:
            llvmir = runtime->llvmGenarate(mir_cc);
            break;
          case CompileStage::EXECUTE:
            returncode = runtime->execute();
            break;
        }
        stage++;
      }
      switch (compile_stage) {
        case CompileStage::AST:
          std::cout << ast->toString() << std::endl;
          break;
        case CompileStage::AST_UNIQUENAME:
          std::cout << ast_u->toString() << std::endl;
          break;
        case CompileStage::TYPEINFOS:
          std::cout << typeinfos.dump() << std::endl;
          break;
        case CompileStage::MIR:
          std::cout << mir->toString() << std::endl;
          break;
        case CompileStage::MIR_CC:
          std::cout << mir_cc->toString() << std::endl;
          break;
        case CompileStage::LLVMIR:
          std::cout << llvmir << std::endl;
          break;
        case CompileStage::EXECUTE:
          std::cerr << "return code: " << returncode << std::endl;
          break;
      }

      // runtime->start();
      // while (true) {
      //   sleep(20);
      // };  // todo : what is best way to wait infinitely? thread?
    } catch (std::exception& e) {
      std::cerr << e.what() << std::endl;
      runtime->stop();
    }
  }
  return 0;
}
