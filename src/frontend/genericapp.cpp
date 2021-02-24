/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "genericapp.hpp"
#include "basic/ast_to_string.hpp"
#include "compiler/codegen/llvm_header.hpp"
#include "runtime/executionengine/executionengine.hpp"
#include "preprocessor/preprocessor.hpp"
namespace {
const std::string_view about_message =
    "mimium - MInimal Musical medIUM, "
    "a programming language as an infrastructure for sound and music";

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

ExecutionEngine getExecutionEngine(std::string_view val) {
  return getEnumByStr(str_to_engine, val);
}

BackEnd getBackEnd(std::string_view val) { return getEnumByStr(str_to_backend, val); }

GenericApp::GenericApp(std::unique_ptr<AppOption> option) : option(std::move(option)) {}

std::ostream& GenericApp::printAbout(std::ostream& out) {
  out << about_message << std::endl;
  out << "version ";
  return printVersion(out);
}

std::ostream& GenericApp::printVersion(std::ostream& out) {
  out << MIMIUM_VERSION;
#ifdef MIMIUM_BUILD_DEBUG
  out << "(debug build)";
#endif
  out << std::endl;
  return out;
}

volatile std::sig_atomic_t GenericApp::signal_status = 0;  // NOLINT

void GenericApp::handleSignal(int signal) { GenericApp::signal_status = signal; }

bool GenericApp::compileMainLoop(Compiler& compiler, const CompileOption& option,
                                 const std::optional<Source>& input,
                                 const std::optional<fs::path>& output_path) {
  auto stage = option.stage;
  compiler.setFilePath(input ? fs::absolute(input.value().filepath).string() : "/stdin");
  // auto preprocessor_path = input ? input.value().filepath.parent_path() : fs::current_path();
  Preprocessor preprocessor(fs::current_path());
  std::stringstream iss;
  if (input) {
    auto newsource = preprocessor.process(input.value().filepath);
    iss << newsource.source;
  } else {
    Logger::debug_log(
        "Reading from stdin. If you are typing from terminal, type Ctrl+D to finish input. ",
        Logger::INFO);
  }
  std::istream& in = input ? iss : std::cin;
  auto ast = compiler.loadSource(in);

  std::ofstream fout;
  if (output_path) { fout.open(output_path.value()); }

  std::ostream& out = output_path ? fout : std::cout;

  if (stage == CompileStage::Parse) {
    out << *ast << std::endl;
    return false;
  }
  auto ast_u = compiler.renameSymbols(ast);
  if (stage == CompileStage::SymbolRename) {
    out << *ast_u << std::endl;
    return false;
  }
  auto& typeinfos = compiler.typeInfer(ast_u);
  if (stage == CompileStage::TypeInference) {
    out << typeinfos.toString() << std::endl;
    return false;
  }
  mir::blockptr mir = compiler.generateMir(ast_u);
  if (stage == CompileStage::MirEmit) {
    out << mir::toString(mir) << std::endl;
    return false;
  }
  auto mir_cc = compiler.closureConvert(mir);
  auto funobjs = compiler.collectMemoryObjs(mir_cc);
  if (stage == CompileStage::ClosureConvert) {
    out << mir::toString(mir_cc) << std::endl;
    return false;
  }
  compiler.generateLLVMIr(mir_cc, funobjs);
  if (stage == CompileStage::Codegen) {
    compiler.dumpLLVMModule(out);
    return false;
  }

  if (output_path) { dynamic_cast<std::ofstream&>(out).close(); }
  return true;
}

int GenericApp::runtimeMainLoop(const RuntimeOption& option, const fs::path& input_path,
                                FileType inputtype, const std::optional<fs::path>& output_path) {
  std::unique_ptr<mimium::ExecutionEngine> exec_engine=nullptr;
  std::unique_ptr<Runtime> runtime=nullptr;
  try {
    bool optimize = option.optimize_level == OptimizeLevel::ON;
    if (option.engine == ExecutionEngine::LLVM) {
      switch (inputtype) {
        case FileType::MimiumSource:
          exec_engine = std::make_unique<LLVMJitExecutionEngine>(
              compiler->moveLLVMCtx(), compiler->moveLLVMModule(),
              fs::absolute(input_path).string(), optimize);
          break;
        case FileType::LLVMIR:
          exec_engine =
              std::make_unique<LLVMJitExecutionEngine>(fs::absolute(input_path).string(), optimize);
          break;
        case FileType::MimiumMir:
          throw std::runtime_error("MIR Parser is not available yet.");
          return -1;
        default: throw std::runtime_error("Unknown File Type"); return -1;
      }
      runtime =
          std::make_unique<Runtime>(std::make_unique<AudioDriverRtAudio>(), std::move(exec_engine));
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
    this->compiler = std::make_unique<Compiler>();
    bool should_compile = true;
    bool should_run = false;
    if (option->input) {
      auto type = option->input.value().filetype;
      if (type != FileType::MimiumSource) { should_compile = false; }
      if (type == FileType::LLVMIR) { should_run = true; }
    }
    if (should_compile) {
      should_run =
          compileMainLoop(*compiler, option->compile_option, option->input, option->output_path);
    }

    int res = 0;
    if (should_run) {
      auto inpath = option->input ? option->input.value().filepath : fs::path("/stdin");
      auto intype = option->input ? option->input.value().filetype : FileType::MimiumSource;
      // auto outpath = option->output_path.value_or(fs::path("/stdout"));
      res = runtimeMainLoop(option->runtime_option, inpath, intype,
                            option->output_path.value_or(fs::path("/stdout")));
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