#include "audiodriver.hpp"

#include <memory>

namespace mimium {
AudioDriver::AudioDriver() {
  try {
    rtaudio = std::make_shared<RtAudio>();
  } catch (RtAudioError& e) {
    e.printMessage();
  }

  if (rtaudio->getDeviceCount() < 1) {
    Logger::debug_log("No audio devices found!", Logger::WARNING);
  }
  parameters.deviceId = rtaudio->getDefaultOutputDevice();
  parameters.nChannels = channels;
  parameters.firstChannel = 0;
}

bool AudioDriver::setCallback(RtAudioCallback cb, void* ud) {
  callback = cb;
  userdata = ud;
  return true;
}

bool AudioDriver::start() {
  try {
    sample_rate =   rtaudio->getDeviceInfo(parameters.deviceId).preferredSampleRate;
    rtaudio->openStream(&parameters, nullptr, RTAUDIO_FLOAT64, sample_rate,
                        &buffer_frames, callback, userdata);
    std::cerr << rtaudio->getStreamSampleRate() << std::endl;
    auto device = rtaudio->getDeviceInfo(rtaudio->getDefaultOutputDevice());
    std::cerr << device.name << std::endl;
    std::cerr << device.outputChannels << std::endl;
    rtaudio->startStream();
  } catch (RtAudioError& e) {
    e.printMessage();
    return false;
  }
  return true;
}
bool AudioDriver::stop() {
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

int AudioDriver::test_audiocallback(void* output_buffer, void* input_buffer,
                                    unsigned int n_buffer_frames,
                                    double stream_time,
                                    RtAudioStreamStatus status,
                                    void* user_data) {
  auto* tcount = static_cast<double*>(user_data);
  unsigned int i;
  unsigned int j;
  auto* buffer = static_cast<double*>(output_buffer);
  // double *lastValues = (double *) userData;
  if (status != 0u) {
    std::cout << "Stream underflow detected!" << std::endl;
  }
  // Write interleaved audio data.
  for (i = 0; i < n_buffer_frames / 2; i++) {
    for (j = 0; j < 2; j++) {
      buffer[i * 2 + j] = (j) ? std::sin(tcount[0]) : std::sin(tcount[1]);
    }
    tcount[0] = std::fmod(tcount[0] + 0.1, M_PI * 2);
    tcount[1] = std::fmod(tcount[1] + 0.2, M_PI * 2);
  }

  return 0;
}

}  // namespace mimium