#define _USE_MATH_DEFINES

#include <string>
#include <cmath>
#include <unistd.h>

#include "../src/audiodriver.hpp"
    double out[2][512];
    double tcount=0;
    int counter=0;
int main() {
    mimium::AudioDriver audio;

    auto cb = [](void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double streamTime, RtAudioStreamStatus status, void *userData)->int{
            //    unsigned int i, j;
                double *buffer = (double *) outputBuffer;
                // double *lastValues = (double *) userData;
        if ( status )
            std::cout << "Stream underflow detected!" << std::endl;
            // Write interleaved audio data.
        for ( int i=0; i<nBufferFrames; i++ ) {
            for ( int j=0; j<2; j++ ) {
                int index = i*2+j;
            buffer[index] = std::sin(tcount);
            counter++;
            }
            tcount = std::fmod(tcount +0.1,M_PI*2);
        }   

        return 0;
         };
    audio.setCallback(cb,nullptr);
    audio.start();
    sleep(10);
    audio.stop();
}