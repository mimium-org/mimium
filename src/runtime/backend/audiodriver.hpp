#pragma once
#include "runtime/runtime_defs.hpp"
namespace mimium {
class Scheduler;

class AudioDriver {
 protected:
  unsigned int sample_rate = 44100;
  unsigned int buffer_size = 256;  // 256 sample per frames
  unsigned int channels = 2;
  Scheduler& sch;
  void* dspfn_cls_address;
  void* dspfn_memobj_address;

  DspFnType dspfn;

 public:
  AudioDriver() = delete;
  explicit AudioDriver(Scheduler& sch, unsigned int sr, unsigned int bs,
                       unsigned int chs)
      : sample_rate(sr), buffer_size(bs), channels(chs), sch(sch){};
  void setDspFn(DspFnType fn) {
    dspfn = fn;
  }
  void setDspClsAddress(void* address){
      dspfn_cls_address = address;
  }
  void setDspMemObjAddress(void* address){
      dspfn_memobj_address = address;
  }
  virtual ~AudioDriver() = default;

  virtual bool start() = 0;
  virtual bool stop() = 0;
};

};  // namespace mimium