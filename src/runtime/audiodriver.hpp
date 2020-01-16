#pragma once

#include "RtAudio.h"
#include "basic/helper_functions.hpp"
#include "runtime/scheduler.hpp"

namespace mimium {
using DspFnType = double (*)(double, void*);

class Scheduler;
// fn (input_buffer*, output_buffer*,
// samplerate,buffersize,channels,userdata)->status

using AudioCallBackFn = std::function<int(double*, double*, unsigned int,
                                          unsigned int, unsigned int, void*)>;
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
                       unsigned int chs);
  void setDspFn(DspFnType fn, void* cls) {
    dspfn = fn;
    dspfn_cls_address = cls;
  }
  virtual ~AudioDriver() = default;

  virtual bool start() = 0;
  virtual bool stop() = 0;
};
class AudioDriverRtAudio : public AudioDriver {
  struct CallbackData {
    Scheduler* scheduler;
    DspFnType dspfn_ptr;
    void* dspfncls_ptr;
    int64_t timeelapsed;
  } callbackdata;
  std::unique_ptr<RtAudio> rtaudio;
  RtAudio::StreamParameters parameters;
  bool setCallback();

 public:
  explicit AudioDriverRtAudio(Scheduler& sch, unsigned int sr = 48000,
                              unsigned int bs = 256, unsigned int chs = 2);
  ~AudioDriverRtAudio() = default;
  bool start() override;
  bool stop() override;
  static RtAudioCallback callback;
};
};  // namespace mimium