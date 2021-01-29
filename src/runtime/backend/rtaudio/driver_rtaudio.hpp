/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "export.hpp"
#include "runtime/backend/audiodriver.hpp"

class RtAudio;
namespace mimium {
class Scheduler;
//Foward declaration wrapper for nested structs in RtAudio class.
class StreamParametersPrivate;
class StreamOptionsPrivate;

class MIMIUM_DLL_PUBLIC AudioDriverRtAudio : public AudioDriver {
 public:
  explicit AudioDriverRtAudio(int buffer_size = 256);
  ~AudioDriverRtAudio() override = default;
  bool start() override;
  bool stop() override;
  private:
  std::unique_ptr<RtAudio> rtaudio;
  std::unique_ptr<StreamParametersPrivate> rtaudio_params_input;
  std::unique_ptr<StreamParametersPrivate> rtaudio_params_output;
  std::unique_ptr<StreamOptionsPrivate> rtaudio_options;
  bool setCallback();
  unsigned int bufsize_internal;
  std::vector<std::unique_ptr<std::vector<double>>> in_buffer;
  std::vector<std::unique_ptr<std::vector<double>>> out_buffer;

  [[nodiscard]] unsigned int getPreferredSampleRate() const;
  void printStreamInfo()const;
};
}  // namespace mimium