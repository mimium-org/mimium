/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef MIMIUM_VERSION
#define MIMIUM_VERSION "unspecified"
#endif

#include <unistd.h>

#include <csignal>
#include <filesystem>
#include <fstream>
#include <string>
namespace fs = std::filesystem;

#include "basic/helper_functions.hpp"
// #include "cli_tools.cpp"
#include "llvm/Support/CommandLine.h"
namespace cl = llvm::cl;
using Logger = mimium::Logger;
#include "basic/ast_to_string.hpp"
#include "compiler/compiler.hpp"
#include "runtime/JIT/runtime_jit.hpp"
#include "runtime/backend/rtaudio/driver_rtaudio.hpp"

extern "C" {
extern mimium::Runtime* global_runtime;
}
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
  int returncode = 0;
  cl::OptionCategory general_category("General Options", "");
  cl::opt<std::string> input_filename(cl::Positional, cl::desc("<input file>"), cl::init("-"),
                                      cl::cat(general_category));

  cl::opt<CompileStage> compile_stage(
      cl::desc("Printing Debug Infomations"),
      cl::values(
          clEnumValN(CompileStage::AST, "emit-ast", "Emit raw abstarct syntax tree to stdout"),
          clEnumValN(CompileStage::AST_UNIQUENAME, "emit-ast-u",
                     "Emit AST with unique symbol names to stdout"),
          clEnumValN(CompileStage::TYPEINFOS, "emit-types",
                     "emit type information for all variables to stdout"),
          clEnumValN(CompileStage::MIR, "emit-mir", "emit MIR to stdout"),
          clEnumValN(CompileStage::MIR_CC, "emit-mir-cc",
                     "emit MIR after closure convertsion to stdout"),

          clEnumValN(CompileStage::LLVMIR, "emit-llvm", "emit LLVM IR to stdout")),
      cl::cat(general_category));
  compile_stage.setInitialValue(CompileStage::EXECUTE);

  cl::ResetAllOptionOccurrences();
  cl::SetVersionPrinter([](llvm::raw_ostream& out) {
    out << "mimium version:" << MIMIUM_VERSION;
#ifdef MIMIUM_BUILD_DEBUG
    out << "(debug build)";
#endif
    out << "\n";
  });
  cl::HideUnrelatedOptions(general_category);
  cl::ParseCommandLineOptions(argc, argv,
                              "mimium - MInimal Musical medIUM, a programming language as an "
                              "infrastructure for sound and music\n");  // launch cli helper

  fs::path filename = input_filename.c_str();
  auto abspath = fs::absolute(filename).string();
  std::ifstream input(abspath.c_str());
  signal(SIGINT, signalHandler);
  Logger::current_report_level = Logger::INFO;
  auto tmpfilename = fs::exists(filename) ? "" : abspath;
  auto compiler = std::make_unique<mimium::Compiler>();
  std::unique_ptr<mimium::Runtime_LLVM> runtime;
  shutdown_handler = [&runtime](int /*signal*/) {
    runtime->getAudioDriver()->stop();
    std::cerr << "Interuppted by key" << std::endl;
    exit(0);
  };
  if (!input.good()) {
    Logger::debug_log("Failed to find file " + abspath, Logger::ERROR_);
    // filename is empty:enter repl mode
  } else {  // try to parse and exec input file
    try {
      Logger::debug_log("Opening " + abspath, Logger::INFO);
      compiler->setFilePath(abspath);

      auto stage = compile_stage.getValue();
      do {
        auto ast = compiler->loadSourceFile(abspath);
        if (stage == CompileStage::AST) {
          std::cout << *ast << std::endl;
          break;
        }
        auto ast_u = compiler->renameSymbols(ast);
        if (stage == CompileStage::AST_UNIQUENAME) {
          std::cout << *ast_u << std::endl;
          break;
        }
        auto& typeinfos = compiler->typeInfer(ast_u);
        if (stage == CompileStage::TYPEINFOS) {
          std::cout << typeinfos.toString() << std::endl;
          break;
        }
        mir::blockptr mir = compiler->generateMir(ast_u);
        if (stage == CompileStage::MIR) {
          std::cout << mir::toString(mir) << std::endl;
          break;
        }
        auto mir_cc = compiler->closureConvert(mir);
        auto funobjs = compiler->collectMemoryObjs(mir_cc);
        if (stage == CompileStage::MIR_CC) {
          std::cout << mir::toString(mir_cc) << std::endl;
          break;
        }
        auto& llvm_module = compiler->generateLLVMIr(mir_cc, funobjs);
        if (stage == CompileStage::LLVMIR) {
          llvm_module.print(llvm::outs(), nullptr, false, true);
          break;
        }
        runtime = std::make_unique<mimium::Runtime_LLVM>(
            compiler->moveLLVMCtx(), tmpfilename, std::make_shared<mimium::AudioDriverRtAudio>());
        global_runtime = runtime.get();
        auto llvmmodule = compiler->moveLLVMModule();
        llvmmodule->setDataLayout(runtime->getJitEngine().getDataLayout());
        runtime->executeModule(std::move(llvmmodule));
        runtime->start();  // start() blocks thread until scheduler stops
        returncode = 0;
        break;
      } while (false);


      runtime->getAudioDriver()->stop();
    } catch (std::exception& e) {
      mimium::Logger::debug_log(e.what(), mimium::Logger::ERROR_);
      runtime->getAudioDriver()->stop();

      returncode = 1;
    }
  }
  std::cerr << "return code: " << returncode << std::endl;
  return returncode;
}
