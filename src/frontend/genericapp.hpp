/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <csignal>
#include <functional>
#include <iostream>
#include <memory>
#include "../libmain.hpp"

#include <csignal>
#include <filesystem>
#include <fstream>
#include <string>
namespace fs = std::filesystem;

namespace mimium::app {

constexpr std::string_view mmm_ext = ".mmm";
constexpr std::string_view ll_ext = ".ll";
constexpr std::string_view bc_ext = ".bc";

template <typename ENUMTYPE>
auto getEnumByStr(const std::unordered_map<std::string_view, ENUMTYPE>& map, std::string_view val) {
  static_assert(static_cast<int>(ENUMTYPE::Invalid) == -1);
  auto iter = map.find(val);
  if (iter != map.cend()) { return iter->second; }
  return ENUMTYPE::Invalid;
}

enum class FileType {
  Invalid = -1,
  MimiumSource = 0,
  MimiumMir,  // currently not used
  LLVMIR,
};
FileType getFileTypeByExt(std::string_view ext);
std::pair<fs::path, FileType> getFilePath(std::string_view val);

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
  ExecutionEngine engine;
  BackEnd backend;
  OptimizeLevel optimize_level;
};
struct AppOption {
  CompileOption compile_option;
  RuntimeOption runtime_option;
  fs::path input_path;
  FileType input_type;
  std::optional<fs::path> output_path;
  bool is_verbose = false;
};

class GenericApp {
 public:
  explicit GenericApp(AppOption& options);

  static std::ostream& printAbout(std::ostream& out = std::cerr);

  static std::ostream& printVersion(std::ostream& out = std::cout);
  // Main Loop. return value is main return code.
  int run();

  static volatile std::sig_atomic_t signal_status;  // NOLINT
  [[nodiscard]] const auto& getOption() const { return option; };

 private:
  static void handleSignal(int signal);

  static std::vector<std::string_view> initRawArgs(int argc, char** argv);
  std::unique_ptr<Compiler> compiler;
  // Compiler Main Loop. If runtime should start, return 1.
  // If compiler should emit result and quit app, return 0.
  static bool compileMainLoop(Compiler& compiler, const CompileOption& option,
                              const fs::path& input_path, FileType inputtype,
                              std::optional<fs::path>& output_path);
  int runtimeMainLoop(const RuntimeOption& option, const fs::path& input_path, FileType inputtype,
                      std::optional<fs::path>& output_path);
  AppOption option;
};

}  // namespace mimium::app