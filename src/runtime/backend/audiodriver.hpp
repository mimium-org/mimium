/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <memory>
#include "runtime/runtime.hpp"

namespace mimium {

class AudioDriver {
 protected:
  Scheduler sch;
  unsigned int buffer_size;  // 256 sample per frames
  unsigned int sample_rate;
  unsigned int channels;
  void* dspfn_cls_address = nullptr;
  void* dspfn_memobj_address = nullptr;
  DspFnType dspfn = nullptr;

 public:
  AudioDriver() = delete;
  explicit AudioDriver(unsigned int bs = 256,
                       unsigned int sr = 44100, unsigned int chs = 2)
      : buffer_size(bs), sample_rate(sr), channels(chs), sch(){};
  void setDspFn(DspFnType fn) { dspfn = fn; }
  void setDspClsAddress(void* address) { dspfn_cls_address = address; }
  void setDspMemObjAddress(void* address) { dspfn_memobj_address = address; }
  virtual ~AudioDriver() = default;
  Scheduler& getScheduler() { return sch; }
  virtual bool start() = 0;
  virtual bool stop() = 0;
  bool processSample(double* input, double* output);
};

};  // namespace mimium