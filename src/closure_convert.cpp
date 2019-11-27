#include "closure_convert.hpp"

namespace mimium {
ClosureConverter::ClosureConverter(TypeEnv& _typeenv):typeenv(_typeenv),capturecount(0){
    env = std::make_shared<Environment>("root",nullptr);
}
ClosureConverter::~ClosureConverter(){

}
// void ClosureConverter::convertFun(std::shared_ptr<FunInst> inst){
//     std::deque<std::string> fvlist;
//     for(auto& childinst : inst->body->instructions){
//         std::visit([&](auto c){c->closureConvert(fvlist, inst->args,env);},childinst);
//     }
//     toplevel_functions.emplace(inst->lv_name,inst);
//     auto capturename = "capture$"+std::to_string(capturecount);
//     Instructions makeclosure = std::make_shared<MakeClosureInst>(capturename,inst->lv_name,inst->freevariables);

//     if(fvlist.size()==0){
//         known_functions.emplace(inst->lv_name,inst);
//     }else{
//         inst->freevariables = std::move(fvlist);
//     }
// }
// bool ClosureConverter::isClosureFunction(std::string str){
//     auto num  = known_functions.count(str);
//     return (num==0); 
// }
// void ClosureConverter::insertMakeClosure(std::shared_ptr<MIRblock> mir, std::deque<Instructions>::iterator it,std::shared_ptr<FcallInst> inst){
//     bool is_ret_func = std::visit(overloaded{
//         [](types::Function c){return true;},
//         [](auto c){return false;}
//     }, inst->type);
//     if(isClosureFunction(inst->fname)){//case of
//     auto fun_ptr = known_functions[inst->fname];
//     capturecount++;
//     mir->instructions.insert(it,std::move(makeclosure)); //
//     inst->ftype = CLOSURE;
//     if(is_ret_func){//in case of return value is function
//         closure_functions.emplace(inst->lv_name,);
//     }
//     }


// };

std::shared_ptr<MIRblock> ClosureConverter::convertRaw(std::shared_ptr<MIRblock> mir){
    
    for(auto it = mir->instructions.begin(),end =mir->instructions.end();it!=end; it++){
        auto& inst = *it;
        std::deque<TypedVal> fvlist;
        auto type =  std::visit([](auto i){return i->type;},inst);
        std::visit([&,this](auto c){
                c->closureConvert(fvlist, shared_from_this(),mir,it);
                },inst);
    }
    return mir;
}

std::shared_ptr<MIRblock> ClosureConverter::moveFunToTop(std::shared_ptr<MIRblock> mir){
  auto& tinsts = mir->instructions;
  auto cit =  tinsts.begin();
  while(cit != tinsts.end()){
    auto& childinst = *cit;
    std::visit(overloaded{
      [&](std::shared_ptr<FunInst> c){c->moveFunToTop(shared_from_this(),mir,cit);cit++;},
      [&cit](auto c){cit++;}
      } ,childinst);//recursively visit;
  }
  return toplevel;
}

std::shared_ptr<MIRblock> ClosureConverter::convert(std::shared_ptr<MIRblock> mir){
    this->toplevel = mir;
    auto c = moveFunToTop(convertRaw(mir));
    return c;
};

}  // namespace mimium