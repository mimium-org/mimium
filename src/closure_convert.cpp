#include "closure_convert.hpp"

namespace mimium {
ClosureConverter::ClosureConverter():capturecount(0){
}
ClosureConverter::~ClosureConverter(){

}
void ClosureConverter::convertFun(std::shared_ptr<FunInst> inst){
    std::deque<std::string> fvlist;
    for(auto& childinst : inst->body->instructions){
        std::visit([&](auto c){c->gatherFreeVariable(fvlist, inst->args);},childinst);
    }
    if(fvlist.size()>0){
        inst->freevariables = std::move(fvlist);
        closure_functions.emplace(inst->lv_name,inst);
    }
}
bool ClosureConverter::isClosureFunction(std::string str){
    auto num  = closure_functions.count(str);
    return (num>0); 
}
void ClosureConverter::insertMakeClosure(std::shared_ptr<MIRblock> mir, std::deque<Instructions>::iterator it,std::shared_ptr<FcallInst> inst){
    if(isClosureFunction(inst->fname)){//case of
    auto fun_ptr = closure_functions[inst->fname];
    auto capturename = "capture$"+std::to_string(capturecount);
    capturecount++;
    Instructions makeclosure = std::make_shared<MakeClosureInst>(capturename,inst->fname,fun_ptr->freevariables);
    mir->instructions.insert(it,std::move(makeclosure)); //
    inst->type = CLOSURE;
    }

};

std::shared_ptr<MIRblock> ClosureConverter::convert(std::shared_ptr<MIRblock> mir){
    for(auto it = mir->instructions.begin(),end =mir->instructions.end();it!=end; it++){
        auto& inst = *it;
        std::visit(overloaded{
            [this](std::shared_ptr<FunInst> inst){
                //check if closure or not
                convertFun(inst);
            },
            [&it,&mir,this](std::shared_ptr<FcallInst> inst){
                //insert MakeClosureInst before fcall if the function is closure
                insertMakeClosure(mir,it,inst); 
            },
            [](auto other){
                //do nothing
            }
        },inst);
    }
    return mir;
};

}  // namespace mimium