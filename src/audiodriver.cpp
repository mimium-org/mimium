#include "audiodriver.hpp"

namespace mimium{
AudioDriver::AudioDriver(){
    try{
        rtaudio = new RtAudio();
        }catch(RtAudioError& e){
            e.printMessage();
        }
        
    if ( rtaudio->getDeviceCount() < 1 ) {
    Logger::debug_log("No audio devices found!" ,Logger::WARNING);

  }
    parameters.deviceId = rtaudio->getDefaultOutputDevice();
    parameters.nChannels = channels;
    parameters.firstChannel = 0;
}

bool AudioDriver::setCallback(RtAudioCallback cb,void* ud){
    callback = cb;
    userdata= ud;
    return 1;
}

bool AudioDriver::start(){
    try{
        rtaudio->openStream(&parameters,NULL,RTAUDIO_FLOAT64,sampleRate,&bufferFrames,callback,(void*)userdata);
        std::cout << rtaudio->getStreamSampleRate()  <<std::endl;
        auto device = rtaudio->getDeviceInfo(rtaudio->getDefaultOutputDevice());
        std::cout <<device.name <<std::endl;
        std::cout<<device.outputChannels<<std::endl;
        rtaudio->startStream();
    }catch(RtAudioError& e){
        e.printMessage();
        return 0;
    }
    return 1;
}
bool AudioDriver::stop(){
    try{
        rtaudio->stopStream();
        if ( rtaudio->isStreamOpen() ) rtaudio->closeStream();
    }catch(RtAudioError& e){
        e.printMessage();
        return 0;
    }
    return 1;
}

int AudioDriver::test_audiocallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double streamTime, RtAudioStreamStatus status, void *userData){
        double* tcount = (double*)userData;
               unsigned int i, j;
                double *buffer = (double *) outputBuffer;
                // double *lastValues = (double *) userData;
        if ( status )
            std::cout << "Stream underflow detected!" << std::endl;
            // Write interleaved audio data.
        for ( i=0; i<nBufferFrames/2; i++ ) {
            for ( j=0; j<2; j++ ) {
            buffer[i*2+j] = (j)?std::sin(tcount[0]):std::sin(tcount[1]);
            }
            tcount[0]= std::fmod(tcount[0]+0.1,M_PI*2);
            tcount[1]= std::fmod(tcount[1]+0.2,M_PI*2);
        }   

        return 0;
         }

}//namespace mimium