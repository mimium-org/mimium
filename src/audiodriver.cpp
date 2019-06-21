#include "audiodriver.hpp"

namespace mimium{
AudioDriver::AudioDriver(){
    if ( rtaudio.getDeviceCount() < 1 ) {
    std::cout << "\nNo audio devices found!\n";
  }
    parameters.deviceId = rtaudio.getDefaultOutputDevice();
    parameters.nChannels = 2;
    parameters.firstChannel = 0;
}

bool AudioDriver::setCallback(RtAudioCallback cb,mimium::Scheduler* ud){
    callback = cb;
    userdata=ud;
    return 1;
}

bool AudioDriver::start(){
    try{
        rtaudio.openStream(&parameters,NULL,RTAUDIO_FLOAT64,sampleRate,&bufferFrames,callback,(void*)userdata);
        rtaudio.startStream();
    }catch(RtAudioError& e){
        e.printMessage();
        return 0;
    }
    return 1;
}
bool AudioDriver::stop(){
    try{
        rtaudio.stopStream();
        if ( rtaudio.isStreamOpen() ) rtaudio.closeStream();
    }catch(RtAudioError& e){
        e.printMessage();
        return 0;
    }
    return 1;
}


}