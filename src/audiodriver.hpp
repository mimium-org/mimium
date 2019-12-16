#pragma once
#include "helper_functions.hpp"
#include "RtAudio.h"
#include <cmath>
#include <memory>

namespace mimium{

class AudioDriver{
    std::shared_ptr<RtAudio> rtaudio;
    RtAudio::StreamParameters parameters;
    unsigned int sample_rate = 44100;
    unsigned int buffer_frames =4096; // 256 sample per frames
    int channels=2;
    // double tcount[2] ={0,0};
    void* userdata;
    // double data[2];
    public:
    AudioDriver();
    ~AudioDriver()=default;
    RtAudioCallback callback=nullptr;

    bool setCallback(RtAudioCallback cb,void* userdata);

    bool start();
    bool stop();
    
    static int test_audiocallback(void* output_buffer, void* input_buffer,
                                    unsigned int n_buffer_frames,
                                    double stream_time,
                                    RtAudioStreamStatus status,
                                    void* user_data);

};
};