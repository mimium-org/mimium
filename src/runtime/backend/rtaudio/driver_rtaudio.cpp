/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "runtime/backend/rtaudio/driver_rtaudio.hpp"

namespace mimium {
AudioDriverRtAudio::AudioDriverRtAudio(int buffer_size)
    : AudioDriver(), bufsize_internal(buffer_size) {
  try {
    rtaudio = std::make_unique<RtAudio>();
  } catch (RtAudioError& e) { e.printMessage(); }

  if (rtaudio->getDeviceCount() < 1) { throw std::runtime_error("No audio devices found!"); }
  rtaudio_params_input.deviceId = rtaudio->getDefaultInputDevice();
  rtaudio_params_output.deviceId = rtaudio->getDefaultOutputDevice();
  rtaudio_params_input.nChannels = getInputDevice().inputChannels;
  rtaudio_params_output.nChannels = getOutputDevice().outputChannels;
  rtaudio_params_input.firstChannel = 0;
  rtaudio_params_output.firstChannel = 0;
}
RtAudioCallback AudioDriverRtAudio::callback = [](void* output, void* input, unsigned int nFrames,
                                                  double time, RtAudioStreamStatus status,
                                                  void* userdata) -> int {
  auto* driver = static_cast<AudioDriverRtAudio*>(userdata);
  // Process interleaved audio data.
  driver->process(static_cast<const double*>(input), static_cast<double*>(output));
  if (status > 0) { Logger::debug_log("Stream underflow detected!", Logger::WARNING); }
  return status;
};

[[nodiscard]] unsigned int AudioDriverRtAudio::getPreferredSampleRate() const {
  auto sr_in = getInputDevice().preferredSampleRate;
  auto sr_out = getOutputDevice().preferredSampleRate;
  if (sr_in != sr_out) {
    Logger::debug_log("input & output sample rates are different. Uses output's samplerate :" +
                          std::to_string(sr_out),
                      Logger::WARNING);
  }
  return sr_out;
}
void AudioDriverRtAudio::printStreamInfo() const {
  std::string deviceinfostr;
  auto indevice = getInputDevice();
  auto outdevice = getOutputDevice();
  deviceinfostr += "Input Audio Device : " + indevice.name;
  deviceinfostr += " - " + std::to_string(indevice.inputChannels) + "chs\n ";
  deviceinfostr += "Output Audio Device : " + outdevice.name;
  deviceinfostr += " - " + std::to_string(outdevice.outputChannels) + "chs\n ";
  deviceinfostr += "Sampling Rate : " + std::to_string(rtaudio->getStreamSampleRate());
  deviceinfostr += " / Buffer Size : " + std::to_string(bufsize_internal);
  Logger::debug_log(deviceinfostr, Logger::INFO);
}
bool AudioDriverRtAudio::start() {
  try {
    rtaudio_options.streamName = "mimium";
    auto sr = getPreferredSampleRate();

    auto p = std::make_unique<AudioDriverParams>(
        AudioDriverParams{static_cast<double>(sr), static_cast<int>(bufsize_internal),
                          static_cast<int>(rtaudio_params_input.nChannels),
                          static_cast<int>(rtaudio_params_output.nChannels)});
    // check parameter are valid
    AudioDriver::setup(std::move(p));
    AudioDriver::start();
    rtaudio->openStream(&rtaudio_params_output, &rtaudio_params_input, RTAUDIO_FLOAT64, sr,
                        &bufsize_internal, AudioDriverRtAudio::callback, this, &rtaudio_options,
                        nullptr);
    params->buffersize = static_cast<int>(bufsize_internal);

    printStreamInfo();

    bool hasdsp = dspfninfos->fn != nullptr;
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