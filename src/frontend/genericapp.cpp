/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "genericapp.hpp"
#include "../basic/ast_to_string.hpp"

namespace {
const std::string_view about_message =
    "mimium - MInimal Musical medIUM, "
    "a programming language as an infrastructure for sound and music";

const std::unordered_map<std::string_view, mimium::app::FileType> str_to_filetype = {
    {mimium::app::mmm_ext, mimium::app::FileType::MimiumSource},
    {mimium::app::ll_ext, mimium::app::FileType::LLVMIR},
    {mimium::app::bc_ext, mimium::app::FileType::LLVMIR},
};

const std::unordered_map<std::string_view, mimium::app::ExecutionEngine> str_to_engine = {
    {"llvm", mimium::app::ExecutionEngine::LLVM},
    {"LLVM", mimium::app::ExecutionEngine::LLVM},
    {"interpreter", mimium::app::ExecutionEngine::Interpreter},
    {"wasm", mimium::app::ExecutionEngine::WebAssembly},
    {"WebAssembly", mimium::app::ExecutionEngine::WebAssembly},
};

const std::unordered_map<std::string_view, mimium::app::BackEnd> str_to_backend = {
    {"rtaudio", mimium::app::BackEnd::RtAudio},
    {"api", mimium::app::BackEnd::API},
    {"test", mimium::app::BackEnd::Test},
};

}  // namespace

namespace mimium::app {

FileType getFileTypeByExt(std::string_view ext) { return getEnumByStr(str_to_filetype, ext); }

std::pair<fs::path, FileType> getFilePath(std::string_view val) {
  fs::path res(val);
  auto type = getFileTypeByExt(res.extension().string());
  if (type == FileType::Invalid) {
    throw std::runtime_error("Unknown file type. Expected either of .mmm, .ll or .bc");
  }
  return std::pair(res, type);
}

ExecutionEngine getExecutionEngine(std::string_view val) {
  return getEnumByStr(str_to_engine, val);
}

BackEnd getBackEnd(std::string_view val) { return getEnumByStr(str_to_backend, val); }

GenericApp::GenericApp(AppOption& option) : option(option) {}

std::ostream& GenericApp::printAbout(std::ostream& out) {
  out << about_message << std::endl;
  out << "version ";
  printVersion(out) << std::endl;
  return out;
}

std::ostream& GenericApp::printVersion(std::ostream& out) {
  out << MIMIUM_VERSION;
#ifdef MIMIUM_BUILD_DEBUG
  out << "(debug build)";
#endif
  return out;
}

volatile std::sig_atomic_t GenericApp::signal_status = 0;//NOLINT

void GenericApp::handleSignal(int signal) { GenericApp::signal_status = signal; }

bool GenericApp::compileMainLoop(Compiler& compiler, const CompileOption& option,
                                 const fs::path& input_path, FileType  /*inputtype*/,
                                 std::optional<fs::path>& output_path) {
  compiler.setFilePath(input_path);
  std::ostream* out = nullptr;
  if (output_path) {
    std::ofstream fout;
    fout.open(output_path.value());
    out = &fout;
  } else {
    out = &std::cout;
  }
  assert(out != nullptr);

  auto stage = option.stage;

  auto ast = compiler.loadSourceFile(fs::absolute(input_path));
  if (stage == CompileStage::Parse) {
    *out << *ast << std::endl;
    return false;
  }
  auto ast_u = compiler.renameSymbols(ast);
  if (stage == CompileStage::SymbolRename) {
    *out << *ast_u << std::endl;
    return false;
  }
  auto& typeinfos = compiler.typeInfer(ast_u);
  if (stage == CompileStage::TypeInference) {
    *out << typeinfos.toString() << std::endl;
    return false;
  }
  mir::blockptr mir = compiler.generateMir(ast_u);
  if (stage == CompileStage::MirEmit) {
    *out << mir::toString(mir) << std::endl;
    return false;
  }
  auto mir_cc = compiler.closureConvert(mir);
  auto funobjs = compiler.collectMemoryObjs(mir_cc);
  if (stage == CompileStage::ClosureConvert) {
    *out << mir::toString(mir_cc) << std::endl;
    return false;
  }
  compiler.generateLLVMIr(mir_cc, funobjs);
  if (stage == CompileStage::Codegen) {
    compiler.dumpLLVMModule(*out);
    return false;
  }

  if (output_path) { dynamic_cast<std::ofstream*>(out)->close(); }
  return true;
}

int GenericApp::runtimeMainLoop(const RuntimeOption& option, const fs::path& input_path,
                                FileType inputtype, std::optional<fs::path>&  /*output_path*/) {
  std::unique_ptr<Runtime> runtime;
  try {
    auto backend = std::make_unique<AudioDriverRtAudio>();
    bool optimize = option.optimize_level == OptimizeLevel::ON;
    if (option.engine == ExecutionEngine::LLVM) {
      switch (inputtype) {
        case FileType::MimiumSource:
          runtime = std::make_unique<Runtime_LLVM>(
              compiler->moveLLVMCtx(), compiler->moveLLVMModule(), fs::absolute(input_path.string()),
              std::move(backend), optimize);
          break;
        case FileType::LLVMIR:
          runtime = std::make_unique<Runtime_LLVM>(fs::absolute(input_path).string(), std::move(backend),
                                                   optimize);
          break;
        case FileType::MimiumMir:
          throw std::runtime_error("MIR Parser is not available yet.");
          return -1;
        default: throw std::runtime_error("Unknown File Type"); return -1;
      }
      runtime->runMainFun();
      runtime->start();  // start() blocks thread until scheduler stops
      return 0;
    }
    throw std::runtime_error("Execution engine other than llvm is not available yet");
  } catch (std::exception& e) {
    if (runtime) { runtime->getAudioDriver().stop(); }
    std::cerr << e.what() << std::endl;
    return -1;
  } catch (...) {
    if (runtime) { runtime->getAudioDriver().stop(); }
    std::cerr << "caught unknown error." << std::endl;
    return -1;
  }
}

int GenericApp::run() {
  try {
    compiler = std::make_unique<Compiler>();
    bool should_run = false;
    if (option.input_type == FileType::MimiumSource) {
      should_run = compileMainLoop(*compiler, option.compile_option, option.input_path,
                                   option.input_type, option.output_path);
    }
    if (option.input_type == FileType::LLVMIR) { should_run = true; }

    int res = 0;
    if (should_run) {
      auto backend = std::make_unique<AudioDriverRtAudio>();
      bool optimize = option.runtime_option.optimize_level == OptimizeLevel::ON;
      res = runtimeMainLoop(option.runtime_option, option.input_path, option.input_type,
                            option.output_path);
    }
    return res;
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;

    return -1;
  } catch (...) {
    std::cerr << "caught unknown error." << std::endl;
    return -1;
  }
}

}  // namespace mimium::app