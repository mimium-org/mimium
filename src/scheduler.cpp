#include "scheduler.hpp"


void mimium::Scheduler::start(){
    audio.setCallback(mimium::Scheduler::audioCallback,this);
    audio.start();
}

void mimium::Scheduler::stop(){
    audio.stop();

}
void mimium::Scheduler::incrementTime(){
    time++;

    if(lasttask_index->first-time<0){
        interpreter->interpretListAst(lasttask_index->second);
        int finaltime= tasks.rbegin()->first;
        if(lasttask_index->first < finaltime){
        lasttask_index++;
        }else{
            audio.stop();
        }
    }
}

void mimium::Scheduler::addTask(int time,AST_Ptr fn){
    // fn->set_time(-1); //remove time to execute 
    tasks.insert(std::make_pair(time,fn));
    if(tasks.size()==1){
        lasttask_index = tasks.begin();
            nexttask_time = time;
    }
}
int mimium::Scheduler::audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData){
        auto sch= (mimium::Scheduler*) userData;
        double* outputBuffer_d[nBufferFrames];
        if ( status )
            std::cout << "Stream underflow detected!" << std::endl;
            // Write interleaved audio data.
        for (int i=0; i<nBufferFrames; i++ ) {
            sch->incrementTime();
            outputBuffer_d[i] = 0;
        }   
            outputBuffer = outputBuffer_d;
        return 0;
        }