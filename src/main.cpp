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
#include "runtime.hpp"

std::function<void(int)> shutdown_handler;
void signalHandler(int signo) { shutdown_handler(signo); }

auto main(int argc, char** argv) -> int {
  enum CompileStage { AST = 0, AST_UNIQUENAME, TYPEINFOS, MIR, MIR_CC, LLVMIR };

  cl::OptionCategory general_category("General Options", "");
  cl::opt<std::string> input_filename(cl::Positional, cl::desc("<input file>"),
                                      cl::init("-"), cl::cat(general_category));

  cl::opt<CompileStage> compile_stage(
      cl::desc("Printing Debug Infomations"),
      cl::values(
          clEnumValN(AST, "emit-ast", "Enable trivial optimizations"),
          clEnumValN(AST_UNIQUENAME, "emit-ast-u",
                     "Enable default optimizations"),
          clEnumValN(TYPEINFOS, "emit-types",
                     "emit type information for all variables to stdout"),
          clEnumValN(MIR, "emit-mir", "emit MIR to stdout"),
          clEnumValN(MIR_CC, "emit-mir-cc",
                     "emit MIR after closure convertsion to stdout"),

          clEnumValN(LLVMIR, "emit-llvm", "emit LLVM IR to stdout")),
      cl::cat(general_category));
  cl::opt<bool> snd_file("sndfile",
                         cl::desc("write out a sound file as an output"));
  cl::ResetAllOptionOccurrences();

  cl::HideUnrelatedOptions(general_category);
  cl::ParseCommandLineOptions(argc, argv, "Mimium\n");  // launch cli helper

  std::ifstream input(input_filename.c_str());
  signal(SIGINT, signalHandler);
  mimium::Logger::current_report_level = mimium::Logger::INFO;
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
    std::cout << "start" << std::endl;

    while (std::getline(std::cin, line)) {
      runtime->clearDriver();
      runtime->loadSource(line);
      // now load source is void function, how to debug print?
      // std::cout << resstr << std::endl;
    }
  } else {  // try to parse and exec input file
    try {
      std::cerr << "Opening: " << input_filename.c_str() << std::endl;
      int stage = 0;
      AST_Ptr ast, ast_u;
      std::shared_ptr<mimium::MIRblock> mir, mir_cc;
      mimium::TypeEnv& typeinfos = runtime->getTypeEnv();
      std::string llvmir;
      while (stage <= static_cast<int>(compile_stage)) {
        switch (static_cast<CompileStage>(stage)) {
          case AST:
            ast = runtime->loadSourceFile(input_filename.c_str());
            break;
          case AST_UNIQUENAME:
            ast_u = runtime->alphaConvert(ast);
            break;
          case TYPEINFOS:
            typeinfos = runtime->typeInfer(ast_u);
            break;
          case MIR:
            mir = runtime->kNormalize(ast_u);
            break;
          case MIR_CC:
            mir_cc = runtime->closureConvert(mir);
            break;
          case LLVMIR:
            llvmir = runtime->llvmGenarate(mir_cc);
            break;
        }
        stage++;
      }
      switch (compile_stage) {
        case AST:
          std::cout << ast->toString() << std::endl;
          break;
        case AST_UNIQUENAME:
          std::cout << ast_u->toString() << std::endl;
          break;
        case TYPEINFOS:
          std::cout << typeinfos.dump() << std::endl;
          break;
        case MIR:
          std::cout << mir->toString() << std::endl;
          break;
        case MIR_CC:
          std::cout << mir_cc->toString() << std::endl;
          break;
        case LLVMIR:
          std::cout << llvmir << std::endl;
          break;
        default:
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
