/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <csignal>
#include <functional>
#include <iostream>
#include <memory>
#include "../libmain.hpp"
#include "basic/filereader.hpp"

#include <csignal>
#include <filesystem>
#include <fstream>
#include <string>
namespace fs = std::filesystem;

namespace mimium::app {


enum class CompileStage {
  Parse = 0,
  SymbolRename,
  TypeInference,
  MirEmit,
  ClosureConvert,
  MemobjCollect,
  Codegen,
  Run
};

enum class ExecutionEngine {
  Invalid = -1,
  LLVM = 0,
  // not implemented
  Interpreter,
  WebAssembly
};
ExecutionEngine getExecutionEngine(std::string_view val);

enum class BackEnd { Invalid = -1, API, Test, RtAudio };
BackEnd getBackEnd(std::string_view val);

enum class OptimizeLevel { ON, OFF };

struct CompileOption {
  CompileStage stage = CompileStage::Run;
};

struct RuntimeOption {
  ExecutionEngine engine = ExecutionEngine::LLVM;
  BackEnd backend = BackEnd::RtAudio;
  OptimizeLevel optimize_level;
};
struct AppOption {
  CompileOption compile_option;
  RuntimeOption runtime_option;
  std::optional<Source> input=std::nullopt;
  std::optional<fs::path> output_path;
  bool is_verbose = false;
};

class GenericApp {
 public:
  explicit GenericApp(std::unique_ptr<AppOption> options);

  static std::ostream& printAbout(std::ostream& out = std::cerr);

  static std::ostream& printVersion(std::ostream& out = std::cout);
  // Main Loop. return value is main return code.
  int run();

  static volatile std::sig_atomic_t signal_status;  // NOLINT
  [[nodiscard]] const auto& getOption() const { return *option; };

 private:
  std::unique_ptr<Compiler> compiler;
  static void handleSignal(int signal);
  // Compiler Main Loop. If runtime should start, return 1.
  // If compiler should emit result and quit app, return 0.
  static bool compileMainLoop(Compiler& compiler, const CompileOption& option,
                              const std::optional<Source>& input,
                              std::optional<fs::path>& output_path);
  int runtimeMainLoop(const RuntimeOption& option, const fs::path& input_path, FileType inputtype,
                      std::optional<fs::path>& output_path);
  std::unique_ptr<AppOption> option;
};

}  // namespace mimium::app