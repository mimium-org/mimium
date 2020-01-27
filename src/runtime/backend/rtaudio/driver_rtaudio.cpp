#include "runtime/backend/rtaudio/driver_rtaudio.hpp"

namespace mimium {
AudioDriverRtAudio::AudioDriverRtAudio(Scheduler& sch, unsigned int sr,
                                       unsigned int bs, unsigned int chs)
    : AudioDriver(sch, sr, bs, chs),
      callbackdata{&sch, sch.getRuntime().getDspFn(),
                   sch.getRuntime().getDspFnCls(), 0} {
  try {
    rtaudio = std::make_unique<RtAudio>();
  } catch (RtAudioError& e) {
    e.printMessage();
  }

  if (rtaudio->getDeviceCount() < 1) {
    throw std::runtime_error("No audio devices found!");
  }
  parameters.deviceId = rtaudio->getDefaultOutputDevice();
  parameters.nChannels = chs;
  parameters.firstChannel = 0;
}

RtAudioCallback AudioDriverRtAudio::callback =
    [](void* output, void* input, unsigned int nFrames, double time,
       RtAudioStreamStatus status, void* userdata) -> int {
  auto data = static_cast<CallbackData*>(userdata);
  auto sch = data->scheduler;
  auto dspfn = data->dspfn_ptr;
  auto dspfn_cls = data->dspfncls_ptr;

  if (sch->isactive) {
    auto* output_buffer_d = static_cast<double*>(output);
    if (status)
      Logger::debug_log("Stream underflow detected!", Logger::WARNING);
    // Write interleaved audio data.
    for (int i = 0; i < nFrames; i++) {
      auto shouldstop = sch->incrementTime();
      if (shouldstop) {
        sch->stop();
        break;
      }
      if (dspfn != nullptr) {
        double res = dspfn((double)data->timeelapsed, dspfn_cls);
        output_buffer_d[i * 2] = res;
        output_buffer_d[i * 2 + 1] = res;
      }
      ++data->timeelapsed;
    }
  }
  return 0;
};

bool AudioDriverRtAudio::start() {
  try {
    callbackdata.dspfn_ptr = dspfn;
    callbackdata.dspfncls_ptr = dspfn_cls_address;
    sample_rate =
        rtaudio->getDeviceInfo(parameters.deviceId).preferredSampleRate;
    rtaudio->openStream(&parameters, nullptr, RTAUDIO_FLOAT64, sample_rate,
                        &buffer_size, AudioDriverRtAudio::callback,
                        &callbackdata);
    std::string deviceinfo = "Audio Device : ";
    auto device = rtaudio->getDeviceInfo(rtaudio->getDefaultOutputDevice());
    deviceinfo += device.name;
    deviceinfo +=
        ", Sampling Rate : " + std::to_string(rtaudio->getStreamSampleRate());
    deviceinfo +=
        ", Output Channels : " + std::to_string(device.outputChannels);
    Logger::debug_log(deviceinfo, Logger::INFO);
    rtaudio->startStream();
  } catch (RtAudioError& e) {
    e.printMessage();
    return false;
  }
  return true;
}
bool AudioDriverRtAudio::stop() {
  try {
    rtaudio->stopStream();
    if (rtaudio->isStreamOpen()) {
      rtaudio->closeStream();
    }
  } catch (RtAudioError& e) {
    e.printMessage();
    return false;
  }
  return true;
}
}  // namespace mimium