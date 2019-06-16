#include <string>
#include <math.h>
#include <unistd.h>

#include "../src/audiodriver.hpp"
    double out[2][512];
    double tcount;
int main() {
    mimium::AudioDriver audio;

    auto cb = [](void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double streamTime, RtAudioStreamStatus status, void *userData)->int{
               unsigned int i, j;
                double *buffer = (double *) outputBuffer;
                double *lastValues = (double *) userData;
        if ( status )
            std::cout << "Stream underflow detected!" << std::endl;
            // Write interleaved audio data.
        for ( i=0; i<nBufferFrames; i++ ) {
            for ( j=0; j<2; j++ ) {
            buffer[i*2+j] = (j)?sin(tcount):sin(tcount*4);
            }
            tcount+=0.1;
            if(tcount>3.141519*2)tcount = 0;
        }   

        return 0;
         };
    audio.setCallback(cb);
    audio.start();
    sleep(10);
    audio.stop();
}