/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef MIMIUM_VERSION
#define MIMIUM_VERSION "unspecified"
#endif

#include "frontend/genericapp.hpp"

#include "frontend/cli.hpp"

int main(int argc, const char** argv) {
  auto app = std::make_unique<mimium::app::cli::CliApp>(argc, argv);
  if (app != nullptr) { return app->run(); }
  return 1;
}