#include "compiler/closure_convert.hpp"

namespace mimium {
ClosureConverter::ClosureConverter(TypeEnv& _typeenv):typeenv(_typeenv),capturecount(0){
    env = std::make_shared<SymbolEnv>("root",nullptr);
}

void ClosureConverter::reset(){
  capturecount=0;
  env = std::make_shared<SymbolEnv>("root",nullptr);
}
ClosureConverter::~ClosureConverter(){

}


std::shared_ptr<MIRblock> ClosureConverter::convertRaw(std::shared_ptr<MIRblock> mir){
    
    for(auto it = mir->instructions.begin(),end =mir->instructions.end();it!=end; it++){
        auto& inst = *it;
        std::vector<TypedVal> fvlist;
        auto type =  std::visit([](auto i){return i->type;},inst);
        std::visit([&,this](auto c){
                c->closureConvert(fvlist, this->shared_from_this(),mir,it);
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