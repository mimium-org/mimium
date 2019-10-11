#pragma once
#include <map>
#include <memory>
#include <utility>
#include <queue>
#include "ast.hpp"
#include "audiodriver.hpp"
#include "helper_functions.hpp"

namespace mimium{

class Scheduler{//scheduler interface
    public:
    explicit Scheduler(std::shared_ptr<ASTVisitor> itp):interpreter(itp),time(0){}
    Scheduler(Scheduler& sch)=default;//copy
    Scheduler(Scheduler&& sch)=default;//move
    Scheduler &operator=(const Scheduler&)=default;
    Scheduler &operator=(Scheduler&&)=default;
    virtual ~Scheduler(){};
    virtual void start() =0;
    virtual void stop()  =0;
    void incrementTime();
    void addTask(int time,AST_Ptr fn);

    protected:
    std::shared_ptr<ASTVisitor> interpreter;
    typedef std::pair<int64_t,AST_Ptr> key_type;
    typedef std::priority_queue<key_type,std::vector<key_type>,std::greater<key_type>> queue_type;
    int64_t time;
    queue_type tasks;
    void executeTask();

};

class SchedulerRT :public Scheduler{

    public:
    struct CallbackData{
        Scheduler* scheduler;
        std::shared_ptr<ASTVisitor> interpreter;
        CallbackData():scheduler(),interpreter(){};
    };
    explicit SchedulerRT(std::shared_ptr<ASTVisitor> itp): Scheduler(itp),audio(){
        userdata.scheduler=this;
        userdata.interpreter=interpreter;
    };
    virtual ~SchedulerRT(){};

    void start() override;
    void stop() override;
    
    inline int64_t getTime(){return time;}
    static int audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,double streamTime, RtAudioStreamStatus status, void* userdata);
    private:
    // std::multimap<int, AST_Ptr>::iterator current_task_index;
    AudioDriver audio;
    CallbackData userdata;
};

}//namespace mimium