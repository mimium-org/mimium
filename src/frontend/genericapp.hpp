/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <csignal>
#include <iostream>
#include "appoptions.hpp"
#include "libmimium.hpp"
#include "export.hpp"
namespace mimium::app {

MIMIUM_DLL_PUBLIC ExecutionEngine getExecutionEngine(std::string_view val);

MIMIUM_DLL_PUBLIC BackEnd getBackEnd(std::string_view val);

class MIMIUM_DLL_PUBLIC GenericApp {
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