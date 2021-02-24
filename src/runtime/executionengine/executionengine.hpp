/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "export.hpp"

namespace mimium {
class Runtime;
class ExecutionEngine {
 public:
  virtual ~ExecutionEngine() = default;
  // Execute global context in mimium source ("mimium_main" function in llvm module).
  // return whether the source has "dsp" function for next step of execution.
  virtual bool runMainFunction(Runtime* runtime_ptr) = 0;
  //optional method run before dsp function starts.
  virtual void preStart(){};
};
}  // namespace mimium