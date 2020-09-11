/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <memory>
#include "runtime/runtime_defs.hpp"
namespace mimium {
class Scheduler;

class AudioDriver {
 protected:
  unsigned int sample_rate = 44100;
  unsigned int buffer_size = 256;  // 256 sample per frames
  unsigned int channels = 2;
  std::shared_ptr<Scheduler> sch;
  void* dspfn_cls_address;
  void* dspfn_memobj_address;

  DspFnType dspfn;

 public:
  AudioDriver() = delete;
  explicit AudioDriver(std::shared_ptr<Scheduler> sch, unsigned int sr, unsigned int bs,
                       unsigned int chs)
      : sample_rate(sr), buffer_size(bs), channels(chs), sch(sch){};
  void setDspFn(DspFnType fn) { dspfn = fn; }
  void setDspClsAddress(void* address) { dspfn_cls_address = address; }
  void setDspMemObjAddress(void* address) { dspfn_memobj_address = address; }
  virtual ~AudioDriver() = default;

  virtual bool start() = 0;
  virtual bool stop() = 0;
};

};  // namespace mimium