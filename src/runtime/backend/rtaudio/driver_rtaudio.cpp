/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "runtime/backend/rtaudio/driver_rtaudio.hpp"
#include "RtAudio.h"

namespace {
const RtAudioCallback callback = [](void* output, void* input, unsigned int nFrames, double time,
                                    RtAudioStreamStatus status, void* userdata) -> int {
  auto* driver = static_cast<mimium::AudioDriverRtAudio*>(userdata);
  // Process interleaved audio data.
  driver->process(static_cast<const double*>(input), static_cast<double*>(output));
  if (status > 0) {
    mimium::Logger::debug_log("Stream underflow detected!", mimium::Logger::WARNING);
  }
  return status;
};

}  // namespace

namespace mimium {
class StreamParametersPrivate {
  RtAudio::StreamParameters param;

 public:
  auto& get() { return param; }
  auto getDeviceInfo(RtAudio& rtaudio) const { return rtaudio.getDeviceInfo(param.deviceId); }
};

class StreamOptionsPrivate {
  RtAudio::StreamOptions opt;

 public:
  auto& get() { return opt; }
};

AudioDriverRtAudio::AudioDriverRtAudio(int buffer_size)
    : AudioDriver(), bufsize_internal(buffer_size) {
  try {
    rtaudio = std::make_unique<RtAudio>();
    rtaudio_params_input = std::make_unique<StreamParametersPrivate>();
    rtaudio_params_output = std::make_unique<StreamParametersPrivate>();
    rtaudio_options = std::make_unique<StreamOptionsPrivate>();

  } catch (RtAudioError& e) { e.printMessage(); }

  if (rtaudio->getDeviceCount() < 1) { throw std::runtime_error("No audio devices found!"); }
  rtaudio_params_input->get().deviceId = rtaudio->getDefaultInputDevice();
  rtaudio_params_output->get().deviceId = rtaudio->getDefaultOutputDevice();
  rtaudio_params_input->get().nChannels =
      rtaudio_params_input->getDeviceInfo(*rtaudio).inputChannels;
  rtaudio_params_output->get().nChannels =
      rtaudio_params_output->getDeviceInfo(*rtaudio).outputChannels;
  rtaudio_params_input->get().firstChannel = 0;
  rtaudio_params_output->get().firstChannel = 0;
}

AudioDriverRtAudio::~AudioDriverRtAudio() = default;

void AudioDriverRtAudio::printStreamInfo() const {
  std::string deviceinfostr;
  auto indevice = rtaudio_params_input->getDeviceInfo(*rtaudio);
  auto outdevice = rtaudio_params_input->getDeviceInfo(*rtaudio);
  deviceinfostr += "Input Audio Device : " + indevice.name;
  deviceinfostr += " - " + std::to_string(indevice.inputChannels) + "chs\n ";
  deviceinfostr += "Output Audio Device : " + outdevice.name;
  deviceinfostr += " - " + std::to_string(outdevice.outputChannels) + "chs\n ";
  deviceinfostr += "Sampling Rate : " + std::to_string(rtaudio->getStreamSampleRate());
  deviceinfostr += " / Buffer Size : " + std::to_string(bufsize_internal);
  Logger::debug_log(deviceinfostr, Logger::INFO);
}
[[nodiscard]] unsigned int AudioDriverRtAudio::getPreferredSampleRate() const {
  auto sr_in = rtaudio_params_input->getDeviceInfo(*rtaudio).preferredSampleRate;
  auto sr_out = rtaudio_params_input->getDeviceInfo(*rtaudio).preferredSampleRate;
  if (sr_in != sr_out) {
    Logger::debug_log("input & output sample rates are different. Uses output's samplerate :" +
                          std::to_string(sr_out),
                      Logger::WARNING);
  }
  return sr_out;
}
bool AudioDriverRtAudio::start() {
  try {
    rtaudio_options->get().streamName = "mimium";
    auto sr = getPreferredSampleRate();

    auto p = std::make_unique<AudioDriverParams>(
        AudioDriverParams{static_cast<double>(sr), static_cast<int>(bufsize_internal),
                          static_cast<int>(rtaudio_params_input->get().nChannels),
                          static_cast<int>(rtaudio_params_output->get().nChannels)});
    // check parameter are valid
    AudioDriver::setup(std::move(p));
    AudioDriver::start();
    rtaudio->openStream(&rtaudio_params_output->get(), &rtaudio_params_input->get(),
                        RTAUDIO_FLOAT64, sr, &bufsize_internal, callback, this,
                        &rtaudio_options->get(), nullptr);
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