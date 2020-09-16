/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <memory>
#include "runtime/runtime.hpp"

namespace mimium {


class AudioDriver {
 protected:
  unsigned int buffer_size;  // 256 sample per frames
  unsigned int sample_rate;
  unsigned int channels;
  Scheduler sch;
  DspFnInfos dspfninfos;

 public:
  AudioDriver() = delete;
  explicit AudioDriver(unsigned int bs = 256,
                       unsigned int sr = 44100, unsigned int chs = 2)
      : buffer_size(bs), sample_rate(sr), channels(chs), sch(){};
  void setDspFnInfos(DspFnInfos&& infos) { 
    dspfninfos = infos;
  }
  virtual ~AudioDriver() = default;
  Scheduler& getScheduler() { return sch; }
  virtual bool start() = 0;
  virtual bool stop() = 0;
  bool processSample(double* input, double* output);
};

};  // namespace mimium