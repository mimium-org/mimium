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

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>

#include "basic/helper_functions.hpp"
// #include "cli_tools.cpp"
#include "llvm/Support/CommandLine.h"
namespace cl = llvm::cl;
using Logger = mimium::Logger;
// #include "compiler/compiler.hpp"
#include "runtime/JIT/runtime_jit.hpp"
#include "runtime/backend/rtaudio/driver_rtaudio.hpp"

extern "C" {
extern mimium::Runtime_LLVM* global_runtime;
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
  cl::opt<std::string> input_filename(cl::Positional, cl::desc("<input file>"), cl::init("-"),
                                      cl::cat(general_category));

  cl::ResetAllOptionOccurrences();

  cl::HideUnrelatedOptions(general_category);
  cl::ParseCommandLineOptions(argc, argv, "Mimium\n");  // launch cli helper
  cl::SetVersionPrinter([](llvm::raw_ostream& out) {
    out << "mimium version:" << MIMIUM_VERSION;
#ifdef MIMIUM_BUILD_DEBUG
    out << "(debug build)";
#endif
    out << "\n";
  });
  std::ifstream input(input_filename.c_str());
  fs::path filename = input_filename.c_str();
  auto abspath = fs::absolute(filename).string();
  signal(SIGINT, signalHandler);
  Logger::current_report_level = Logger::INFO;
  auto tmpfilename = fs::exists(filename) ? "" :abspath;  auto runtime = std::make_shared<mimium::Runtime_LLVM>(
      tmpfilename, std::make_shared<mimium::AudioDriverRtAudio>());
  shutdown_handler = [&runtime](int /*signal*/) {
    runtime->getAudioDriver()->stop();

    std::cerr << "Interuppted by key" << std::endl;
    exit(0);
  };
  global_runtime = runtime.get();
  llvm::SMDiagnostic errorreporter;
  if (!input.good()) {
    Logger::debug_log("Specify file name, repl mode is not implemented yet", Logger::ERROR_);
    // filename is empty:enter repl mode
  } else {  // try to parse and exec input file
    try {
      Logger::debug_log("Opening " + abspath, Logger::INFO);

      auto m = llvm::parseIRFile(abspath, errorreporter, runtime->getLLVMContext());
      runtime->executeModule(std::move(m));
      runtime->start();  // start() blocks thread until scheduler stops
      returncode = 0;

    } catch (std::exception& e) {
      mimium::Logger::debug_log(e.what(), mimium::Logger::ERROR_);
      runtime->getAudioDriver()->stop();

      returncode = 1;
    }
  }
  llvm::errs() << "return code: " << returncode << "\n";
  return returncode;
}
