/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "export.hpp"
#include "RtAudio.h"
#include "runtime/backend/audiodriver.hpp"

namespace mimium {
class Scheduler;

class MIMIUM_DLL_PUBLIC AudioDriverRtAudio : public AudioDriver {
  std::unique_ptr<RtAudio> rtaudio;
  RtAudio::StreamParameters rtaudio_params_input;
  RtAudio::StreamParameters rtaudio_params_output;
  RtAudio::StreamOptions rtaudio_options;
  bool setCallback();
  unsigned int bufsize_internal;
  std::vector<std::unique_ptr<std::vector<double>>> in_buffer;
  std::vector<std::unique_ptr<std::vector<double>>> out_buffer;

  [[nodiscard]] unsigned int getPreferredSampleRate() const;
  auto getInputDevice() const { return rtaudio->getDeviceInfo(rtaudio_params_input.deviceId); }
  auto getOutputDevice() const { return rtaudio->getDeviceInfo(rtaudio_params_output.deviceId); }
  void printStreamInfo()const;
 public:
  explicit AudioDriverRtAudio(int buffer_size = 256);
  ~AudioDriverRtAudio() override = default;
  bool start() override;
  bool stop() override;
  static RtAudioCallback callback;
};
}  // namespace mimium