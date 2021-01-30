/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "scheduler.hpp"

namespace mimium {

bool Scheduler::Greater::operator()(const key_type& l, const key_type& r) const {
  return l.first > r.first;
}

// return value: shouldstop
bool Scheduler::incrementTime() {
  bool hastask = !tasks.empty();
  bool shouldplay = hasdsp || hastask;
  if (!shouldplay) { return true; }

  time += 1;
  if (hastask && time > tasks.top().first) { executeTask(tasks.top().second); }
  return false;
}
void Scheduler::addTask(double time, void* addresstofn, double arg, void* addresstocls) {
  tasks.emplace(static_cast<int64_t>(time), TaskType{addresstofn, arg, addresstocls});
}

void Scheduler::executeTask(const TaskType& task) {
  // todo
  const auto& [addresstofn, arg, addresstocls] = task;

  if (addresstocls == nullptr) {
    auto fn = reinterpret_cast<void (*)(double)>(addresstofn);//NOLINT
    fn(arg);
  } else {
    auto fn = reinterpret_cast<void (*)(double, void*)>(addresstofn);//NOLINT
    fn(arg, addresstocls);
  }
  tasks.pop();
  if (tasks.empty() && !hasdsp) {
    stop();
  } else {
    // recursive call until all tasks have been done!
    if (time > tasks.top().first) { this->executeTask(tasks.top().second); }
  }
}

void Scheduler::start(bool hasdsp) { this->hasdsp = hasdsp; }

void Scheduler::stop() {
  {
    std::lock_guard<std::mutex> lock(wc.mtx);
    wc.isready = true;
  }
  wc.cv.notify_all();  // notify to exit runtime;
}
}  // namespace mimium