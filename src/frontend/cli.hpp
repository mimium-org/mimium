/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <utility>
#include <unordered_map>

namespace mimium::app{
class GenericApp;
namespace cli{


enum class ArgKind {
  Invalid = -1,
  Output = 0,
  BackEnd,
  ExecutionEngine,
  EmitAst,
  EmitAstUniqueSymbol,
  EmitMir,
  EmitMirClosureCoverted,
  EmitLLVMIR,
  OptimizeLevel,
  ShowVersion,
  ShowHelp,
  Verbose,
};


ArgKind getArgKind(std::string_view str);
bool isArgPaired(ArgKind arg);

enum class CliAppMode { Run, ShowHelp, ShowVersion };

class CliApp {
 public:
  class OptionParser {
   public:
    OptionParser();
    std::pair<AppOption,CliAppMode> operator()(int argc, const char** argv);

   private:
    static std::vector<std::string_view> initRawArgs(int argc, const char** argv);
    void processArgs(ArgKind arg, std::string_view val);
    static bool isArgOption(std::string_view str);
    const std::vector<std::string_view> args;
    AppOption result;
    CliAppMode res_mode;
  };

  explicit CliApp(int argc, const char** argv);
  int run();
  std::ostream& printHelp(std::ostream& out = std::cerr) const;

 private:
  std::unique_ptr<GenericApp> app;
  CliAppMode mode = CliAppMode::Run;
};

}  // namespace cli
  }  // namespace mimium::app