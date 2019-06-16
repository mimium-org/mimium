#pragma once
#include "RtAudio.h"


namespace mimium{

class AudioDriver{
    RtAudio rtaudio;
    RtAudio::StreamParameters parameters;
    unsigned int sampleRate = 44100;
    unsigned int bufferFrames = 512; // 256 sample frames
    double data[2];
    RtAudioCallback callback=nullptr;
    public:
    AudioDriver();
    ~AudioDriver(){};

    bool setCallback(RtAudioCallback cb);
    bool start();
    bool stop();


};
};