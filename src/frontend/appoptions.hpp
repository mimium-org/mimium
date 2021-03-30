#pragma once
#include "basic/filereader.hpp"
#include <optional>
#include <string_view>

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

enum class BackEnd { Invalid = -1, API, Test, RtAudio };

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
  std::optional<Source> input = std::nullopt;
  std::optional<fs::path> output_path;
  bool is_verbose = false;
};
}  // namespace mimium::app