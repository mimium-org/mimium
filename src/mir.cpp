#include "mir.hpp"

namespace mimium {
static std::string join(std::deque<std::string> vec,std::string delim){
    std::string s;
    for (auto& elem : vec) {
    s += elem;
    auto endstr =vec.back();
    if (elem !=  endstr) s += delim;
  }
  return s;
};
std::string MIRblock::toString(){
  std::string str;
  for(int i=0;i< indent_level;i++){
      str+= "  ";//indent
  }
  str+= label+":\n";

  indent_level++;
  for(auto& inst : instructions){
    for(int i=0;i< indent_level;i++){
      str+= "  ";//indent
    }
  
    str += std::visit([](auto val)->std::string{return val->toString();},inst) + "\n";
  }
  indent_level--;
  return str;
}

bool MIRinstruction::isFreeVariable(std::shared_ptr<Environment> env,std::string str){
  auto [isvarset,isfv] = env->isFreeVariable(str);//check local vars

  return isfv;
}
void MIRinstruction::gatherFV_raw(std::deque<std::string>& fvlist ,std::shared_ptr<Environment> env,std::string str){
    if(isFreeVariable(env,str)){
    fvlist.push_back(str);
  }
}

std::string NumberInst::toString() {
  return lv_name + " = " + std::to_string(val);
}
void NumberInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args,std::shared_ptr<Environment> env){
  env->setVariableRaw(lv_name,std::to_string(val));
}

std::string SymbolInst::toString() { return lv_name + " = " + val ; }
void SymbolInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args,std::shared_ptr<Environment> env){
  //do nothing
}
std::string TimeInst::toString() { return lv_name + " = " + val + +"@" + time; }
void TimeInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args,std::shared_ptr<Environment> env){
  gatherFV_raw(fvlist,env,val);
  gatherFV_raw(fvlist,env,time);
  env->setVariableRaw(lv_name,val);
}
std::string OpInst::toString() {
  return lv_name + " = " + lhs + op + rhs;

}
void OpInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args,std::shared_ptr<Environment> env){
  gatherFV_raw(fvlist,env,lhs);
  gatherFV_raw(fvlist,env,rhs);
  env->setVariableRaw(lv_name,"some_op");

}
std::string FunInst::toString() {
  std::string s;
  s += lv_name + " = fun " + join(args," , ");
  if(freevariables.size()>0){
  s+=" fv{ "+ join(freevariables," , ") + " }";
  }
  s+="\n";
  body->indent_level++;
  s += body->toString();
  body->indent_level--;
  
  return s;
}
void FunInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args,std::shared_ptr<Environment> env){
  std::cout << lv_name <<std::endl;
  for(auto& a : args){
    env->setVariableRaw(a,"arg");
  }
  auto tmpenv = env;
  env =  env->createNewChild(lv_name);

  for(auto& childinst : body->instructions){
    std::visit([&](auto c){c->gatherFreeVariable(this->freevariables,this->args,env);} ,childinst);//recursively visit;
  }

  env = tmpenv;
  env->setVariableRaw(lv_name,"some_fun");
  //post process?????
}
std::string MakeClosureInst::toString(){
std::string s;
  s += lv_name + " = makeclosure " + join(captures," , ") ;
  // s += body->toString();
  return s;
}
void MakeClosureInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args,std::shared_ptr<Environment> env){
//do nothing
}
std::string FcallInst::toString(){
    std::string s;
    return lv_name + " = app"+fcalltype_str[type] + " " + fname +" "+ join(args," , ") ;
}
void FcallInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args,std::shared_ptr<Environment> env){
  
  for(auto& a : this->args){
    gatherFV_raw(fvlist,env,a);
  }
  env->setVariableRaw(lv_name,"some_fun");
}

std::string ArrayInst::toString(){
    return lv_name + " = array " + name + " " + join(args," , ");
}

void ArrayInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args,std::shared_ptr<Environment> env){
  for(auto& a : this->args){
    gatherFV_raw(fvlist,env,a);
  }
  env->setVariableRaw(lv_name,"some_array");
}

std::string ArrayAccessInst::toString(){
    return lv_name + " = arrayaccess " + name + " " + index;
}
void ArrayAccessInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args,std::shared_ptr<Environment> env){
    gatherFV_raw(fvlist,env,name);
    gatherFV_raw(fvlist,env,index);
    env->setVariableRaw(lv_name,"some_access");

}

std::string IfInst::toString(){
  std::string s;
  s+= lv_name + " = if " + cond +"\n";
  s+= thenblock->toString();
  s+= elseblock->toString();
  return s;
}
void IfInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args,std::shared_ptr<Environment> env){
    gatherFV_raw(fvlist,env,cond);
    for(auto& theninsts : thenblock->instructions){
      std::visit([&](auto c){c->gatherFreeVariable(fvlist,args,env);},theninsts);
    }
    for(auto& elseinsts : elseblock->instructions){
      std::visit([&](auto c){c->gatherFreeVariable(fvlist,args,env);},elseinsts);
    }
}
std::string ReturnInst::toString(){
    return lv_name + " = return " + val ;
}
void ReturnInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args,std::shared_ptr<Environment> env){
      gatherFV_raw(fvlist,env,val);
}




}  // namespace mimium