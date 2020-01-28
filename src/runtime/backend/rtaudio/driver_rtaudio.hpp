#pragma once
#include "runtime/backend/audiodriver.hpp"
#include "runtime/scheduler/scheduler.hpp"
#include "RtAudio.h"

namespace mimium {
class Scheduler;

class AudioDriverRtAudio : public AudioDriver {
  struct CallbackData {
    Scheduler* scheduler;
    DspFnType dspfn_ptr;
    void* dspfncls_ptr;
    void* dspfn_memobj_ptr;
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
}  // namespace mimium