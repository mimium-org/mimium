#pragma once
#include <map>
#include <memory>
#include <utility>

#include "ast.hpp"
#include "interpreter.hpp"
#include "audiodriver.hpp"
#include "helper_functions.hpp"

namespace mimium{
class Interpreter; //forward
class Scheduler :public std::enable_shared_from_this<Scheduler>{
    int64_t time;
    int nexttask_time;
    std::multimap<int, AST_Ptr> tasks;
    std::multimap<int, AST_Ptr>::iterator lasttask_index;
    std::shared_ptr<Interpreter> interpreter;
    AudioDriver audio;
    public:
    Scheduler(Interpreter* itp): time(0),interpreter(itp),audio(){
    };
    virtual ~Scheduler(){};

    void start();
    void stop();
    void incrementTime();
    void addTask(int time,AST_Ptr fn);
    static int audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,double streamTime, RtAudioStreamStatus status, void* userdata);
};

};