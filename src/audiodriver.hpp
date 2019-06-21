#pragma once
#include "RtAudio.h"


namespace mimium{
//to convert lambda function to RtAudioCallback
class Scheduler;//forward


class AudioDriver{
    RtAudio rtaudio;
    RtAudio::StreamParameters parameters;
    unsigned int sampleRate = 44100;
    unsigned int bufferFrames = 512; // 256 sample frames
    Scheduler* userdata;
    double data[2];
    public:
    AudioDriver();
    ~AudioDriver(){};
    RtAudioCallback callback=nullptr;

    bool setCallback(RtAudioCallback cb,mimium::Scheduler* userdata);

    bool start();
    bool stop();


};
};