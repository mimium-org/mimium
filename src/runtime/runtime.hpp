/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <list>

#include "basic/helper_functions.hpp"
#include "runtime/runtime_defs.hpp"
#include "runtime/scheduler.hpp"
namespace mimium {
class AudioDriver;

class Runtime {
 public:
  explicit Runtime(std::unique_ptr<AudioDriver> a=nullptr):audiodriver(std::move(a))  {}

  virtual ~Runtime() {
    for (auto&& [address, size] : malloc_container) { free(address); }
  };
  
  virtual void runMainFun()=0;
  virtual void start() = 0;
  auto& getAudioDriver() { return *audiodriver; }
  [[nodiscard]] bool hasDsp() const { return hasdsp; }
  [[nodiscard]] bool hasDspCls() const { return hasdspcls; }
  void push_malloc(void* address, size_t size) { malloc_container.emplace_back(address, size); }

 protected:
  std::unique_ptr<AudioDriver> audiodriver;
  bool hasdsp = false;
  bool hasdspcls = false;
  std::list<std::pair<void*, size_t>> malloc_container{};
};

}  // namespace mimium