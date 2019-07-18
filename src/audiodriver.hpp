#pragma once
#include "helper_functions.hpp"
#include "RtAudio.h"
#include <cmath>

namespace mimium{
//to convert lambda function to RtAudioCallback
class Scheduler;//forward


class AudioDriver{
    RtAudio* rtaudio;
    RtAudio::StreamParameters parameters;
    unsigned int sampleRate = 44100;
    unsigned int bufferFrames = 512; // 256 sample per frames
    int channels=2;
    double tcount[2] ={0,0};
    void* userdata;
    // double data[2];
    public:
    AudioDriver();
    ~AudioDriver(){};
    RtAudioCallback callback=nullptr;

    bool setCallback(RtAudioCallback cb,mimium::Scheduler* userdata);

    bool start();
    bool stop();
    
    static int test_audiocallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double streamTime, RtAudioStreamStatus status, void *userData);

};
};