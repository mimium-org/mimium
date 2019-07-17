#include "audiodriver.hpp"

namespace mimium{
AudioDriver::AudioDriver(){
    if ( rtaudio.getDeviceCount() < 1 ) {
    Logger::debug_log("No audio devices found!" ,Logger::WARNING);

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