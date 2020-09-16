/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "runtime/backend/rtaudio/driver_rtaudio.hpp"

namespace mimium {
AudioDriverRtAudio::AudioDriverRtAudio(unsigned int bs, unsigned int sr, unsigned int chs)
    : AudioDriver(bs, sr, chs) {
  try {
    rtaudio = std::make_unique<RtAudio>();
  } catch (RtAudioError& e) { e.printMessage(); }

  if (rtaudio->getDeviceCount() < 1) { throw std::runtime_error("No audio devices found!"); }
  parameters.deviceId = rtaudio->getDefaultOutputDevice();
  parameters.nChannels = chs;
  parameters.firstChannel = 0;
}
RtAudioCallback AudioDriverRtAudio::callback = [](void* output, void* input, unsigned int nFrames,
                                                  double time, RtAudioStreamStatus status,
                                                  void* userdata) -> int {
  auto* driver = static_cast<AudioDriverRtAudio*>(userdata);

  auto* input_d = static_cast<double*>(input);
  auto* output_d = static_cast<double*>(output);
  if (status > 0) { Logger::debug_log("Stream underflow detected!", Logger::WARNING); }
  // Write interleaved audio data.
  for (int i = 0; i < nFrames; i++) {
    double* output_pos = std::next(output_d, i * driver->channels);
    double* input_pos = std::next(input_d, i * driver->channels);
    driver->processSample(input_pos, output_pos);
  }
  return status;
};

bool AudioDriverRtAudio::start() {
  try {
    sample_rate = rtaudio->getDeviceInfo(parameters.deviceId).preferredSampleRate;
    rtaudio->openStream(&parameters, nullptr, RTAUDIO_FLOAT64, sample_rate, &buffer_size,
                        AudioDriverRtAudio::callback, this);
    std::string deviceinfo = "Audio Device : ";
    auto device = rtaudio->getDeviceInfo(rtaudio->getDefaultOutputDevice());
    deviceinfo += device.name;
    deviceinfo += ", Sampling Rate : " + std::to_string(rtaudio->getStreamSampleRate());
    deviceinfo += ", Output Channels : " + std::to_string(device.outputChannels);
    Logger::debug_log(deviceinfo, Logger::INFO);
    bool hasdsp = dspfninfos.fn != nullptr;
    sch.start(hasdsp);
    rtaudio->startStream();
  } catch (RtAudioError& e) {
    e.printMessage();
    return false;
  }
  return true;
}
bool AudioDriverRtAudio::stop() {
  try {
    sch.stop();
    if (rtaudio->isStreamOpen()) {
      rtaudio->stopStream();
      rtaudio->closeStream();
    }
  } catch (RtAudioError& e) {
    e.printMessage();
    return false;
  }
  return true;
}
}  // namespace mimium