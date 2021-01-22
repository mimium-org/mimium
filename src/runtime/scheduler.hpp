/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <queue>
#include <utility>

#include "basic/helper_functions.hpp"
// #include "sndfile.h"

namespace mimium {
struct TaskType {
  void* addresstofn;
  // int64_t tasktypeid;
  double arg;
  void* addresstocls;
};

class Scheduler {  // scheduler interface
 public:
  explicit Scheduler() : wc() {}

  virtual ~Scheduler() = default;
  virtual void start(bool hasdsp);
  virtual void stop();

  bool hasTask() { return !tasks.empty(); }

  // tick the time and return if scheduler should be stopped
  bool incrementTime();

  // time,address to fun, arg(double), addresstoclosure,
  void addTask(double time, void* addresstofn, double arg, void* addresstocls);

  // if dsp function exists
  bool hasdsp = false;
  [[nodiscard]] auto getTime() const { return time; }
  auto& getWaitController() { return wc; }

 protected:
  using key_type = std::pair<int64_t, TaskType>;
  struct Greater {
    bool operator()(const key_type& l, const key_type& r) const;
  };
  WaitController wc;
  using queue_type = std::priority_queue<key_type, std::vector<key_type>, Greater>;
  int64_t time = 0;
  queue_type tasks;
  virtual void executeTask(const TaskType& task);
};

}  // namespace mimium