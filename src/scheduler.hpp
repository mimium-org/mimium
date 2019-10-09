#pragma once
#include <map>
#include <memory>
#include <utility>
#include <queue>
#include "ast.hpp"
#include "audiodriver.hpp"
#include "helper_functions.hpp"

namespace mimium{
class Scheduler{

    public:
    struct CallbackData{
        Scheduler* scheduler;
        std::shared_ptr<ASTVisitor> interpreter;
        CallbackData():scheduler(),interpreter(){};
    };
    explicit Scheduler(std::shared_ptr<ASTVisitor> itp): time(0),nexttask_time(0),interpreter(itp),audio(){
        userdata.scheduler=this;
        userdata.interpreter=interpreter;
    };
    Scheduler(Scheduler& sch)=default;//copy
    Scheduler(Scheduler&& sch)=default;//move
    Scheduler &operator=(const Scheduler&)=default;
    Scheduler &operator=(Scheduler&&)=default;

    virtual ~Scheduler(){};

    void start();
    void stop();
    void incrementTime();
    void addTask(int time,AST_Ptr fn);
    inline int64_t getTime(){return time;}
    static int audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,double streamTime, RtAudioStreamStatus status, void* userdata);
    private:
    typedef std::pair<int64_t,AST_Ptr> key_type;
    typedef std::priority_queue<key_type,std::vector<key_type>,std::greater<key_type>> queue_type;
    int64_t time;
    int nexttask_time;
    queue_type tasks;
    // std::multimap<int, AST_Ptr>::iterator current_task_index;
    std::shared_ptr<ASTVisitor> interpreter;//weak for cross reference
    AudioDriver audio;
    CallbackData userdata;
    void executeTask();
};

};