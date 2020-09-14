/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "RtAudio.h"
#include "runtime/backend/audiodriver.hpp"

namespace mimium {
class Scheduler;

class AudioDriverRtAudio : public AudioDriver {
  std::unique_ptr<RtAudio> rtaudio;
  RtAudio::StreamParameters parameters;
  bool setCallback();

 public:
  explicit AudioDriverRtAudio(unsigned int bs = 256,
                       unsigned int sr = 44100, unsigned int chs = 2);
  ~AudioDriverRtAudio() override = default;
  bool start() override;
  bool stop() override;
  static RtAudioCallback callback;
};
}  // namespace mimium