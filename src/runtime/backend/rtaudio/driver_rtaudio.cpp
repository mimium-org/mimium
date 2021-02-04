/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "runtime/backend/rtaudio/driver_rtaudio.hpp"
#include "RtAudio.h"

namespace {
const RtAudioCallback callback = [](void* output, void* input, unsigned int n_frames,
                                    double /*time*/, RtAudioStreamStatus status,
                                    void* userdata) -> int {
  auto* driver = static_cast<mimium::AudioDriverRtAudio*>(userdata);
  // Process interleaved audio data.
  driver->process(static_cast<const double*>(input), static_cast<double*>(output),
                  static_cast<int>(n_frames));
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

AudioDriverRtAudio::AudioDriverRtAudio() : AudioDriver() {
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
  deviceinfostr += " / Buffer Size : " + std::to_string(params->audioframesize);
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
std::unique_ptr<AudioDriverParams> AudioDriverRtAudio::getDefaultAudioParameter(
    std::optional<int> samplerate, std::optional<int> framesize) const {
  assert(rtaudio != nullptr && dspfninfos != nullptr);
  auto in_chs = std::min(static_cast<int>(rtaudio_params_input->get().nChannels),
                         this->dspfninfos->in_numchs);
  auto out_chs = std::min(static_cast<int>(rtaudio_params_output->get().nChannels),
                          this->dspfninfos->out_numchs);
  int sr = samplerate.value_or(getPreferredSampleRate());
  int frames = framesize.value_or(AudioDriver::default_framesize);
  return std::make_unique<AudioDriverParams>(AudioDriverParams{
      static_cast<double>(sr), static_cast<int>(frames * sizeof(double)), frames, in_chs, out_chs});
}

bool AudioDriverRtAudio::start() {
  try {
    AudioDriver::start();
    rtaudio_options->get().streamName = "mimium";

    rtaudio_params_input->get().nChannels = params->in_numchs;
    rtaudio_params_output->get().nChannels = params->out_numchs;
    auto* iparam = params->in_numchs > 0 ? &rtaudio_params_input->get() : nullptr;
    auto* oparam = params->out_numchs > 0 ? &rtaudio_params_output->get() : nullptr;
    if (iparam == nullptr && oparam == nullptr) {  // create dummy parameter for no dsp mode
      oparam = &rtaudio_params_output->get();
      rtaudio_params_output->get().nChannels = 1;
    }
    // check parameter are valid
    unsigned int framesize = params->audioframesize;
    rtaudio->openStream(oparam, iparam, RTAUDIO_FLOAT64, params->samplerate, &framesize, callback,
                        this, &rtaudio_options->get(), nullptr);
    printStreamInfo();
    params->audioframesize = framesize;

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