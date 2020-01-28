#pragma once

namespace mimium {
using DspFnType = double (*)(double, void*);
class Scheduler;

class AudioDriver {
 protected:
  unsigned int sample_rate = 44100;
  unsigned int buffer_size = 256;  // 256 sample per frames
  unsigned int channels = 2;
  Scheduler& sch;
  void* dspfn_cls_address;
  DspFnType dspfn;

 public:
  AudioDriver() = delete;
  explicit AudioDriver(Scheduler& sch, unsigned int sr, unsigned int bs,
                       unsigned int chs)
      : sample_rate(sr), buffer_size(bs), channels(chs), sch(sch){};
  void setDspFn(DspFnType fn, void* cls) {
    dspfn = fn;
    dspfn_cls_address = cls;
  }
  virtual ~AudioDriver() = default;

  virtual bool start() = 0;
  virtual bool stop() = 0;
};

};  // namespace mimium