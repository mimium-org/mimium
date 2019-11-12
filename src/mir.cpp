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

bool MIRinstruction::isFreeVariable(std::deque<std::string>& args, std::string str){
  int c =  std::count(args.cbegin(),args.cend(),str);
  return c == 0;
}
void MIRinstruction::gatherFV_raw(std::deque<std::string>& fvlist, std::deque<std::string>& args,std::string str){
    if(isFreeVariable(args,str)){
    fvlist.push_back(str);
  }
}

std::string NumberInst::toString() {
  return lv_name + " = " + std::to_string(val);
}
void NumberInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args){
  //do nothing
}

std::string SymbolInst::toString() { return lv_name + " = " + val ; }
void SymbolInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args){
  //do nothing
}
std::string TimeInst::toString() { return lv_name + " = " + val + +"@" + time; }
void TimeInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args){
  gatherFV_raw(fvlist,args,val);
  gatherFV_raw(fvlist,args,time);
}
std::string OpInst::toString() {
  return lv_name + " = " + lhs + op + rhs;
}
void OpInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args){
  gatherFV_raw(fvlist,args,lhs);
  gatherFV_raw(fvlist,args,rhs);
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
void FunInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args){
  std::cout << lv_name <<std::endl;
  std::deque<std::string> newfvlist;
  for(auto& childinst : body->instructions){
    std::visit([&](auto c){c->gatherFreeVariable(newfvlist,this->args);} ,childinst);//recursively visit;
  }
  this->freevariables = std::move(newfvlist);
  //post process?????
}
std::string MakeClosureInst::toString(){
std::string s;
  s += lv_name + " = makeclosure " + join(captures," , ") ;
  // s += body->toString();
  return s;
}
void MakeClosureInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args){
//do nothing
}
std::string FcallInst::toString(){
    std::string s;
    return lv_name + " = app"+fcalltype_str[type] + " " + fname +" "+ join(args," , ") ;
}
void FcallInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args){
  for(auto& a : this->args){
    gatherFV_raw(fvlist,args,a);
  }
}

std::string ArrayInst::toString(){
    return lv_name + " = array " + name + " " + join(args," , ");
}

void ArrayInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args){
  for(auto& a : this->args){
    gatherFV_raw(fvlist,args,a);
  }
}

std::string ArrayAccessInst::toString(){
    return lv_name + " = arrayaccess " + name + " " + index;
}
void ArrayAccessInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args){
    gatherFV_raw(fvlist,args,name);
    gatherFV_raw(fvlist,args,index);
}

std::string IfInst::toString(){
  std::string s;
  s+= lv_name + " = if " + cond +"\n";
  s+= thenblock->toString();
  s+= elseblock->toString();
  return s;
}
void IfInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args){
    gatherFV_raw(fvlist,args,cond);
    for(auto& theninsts : thenblock->instructions){
      std::visit([&](auto c){c->gatherFreeVariable(fvlist,args);},theninsts);
    }
    for(auto& elseinsts : elseblock->instructions){
      std::visit([&](auto c){c->gatherFreeVariable(fvlist,args);},elseinsts);
    }
}
std::string ReturnInst::toString(){
    return lv_name + " = return " + val ;
}
void ReturnInst::gatherFreeVariable(std::deque<std::string>& fvlist, std::deque<std::string>& args){
      gatherFV_raw(fvlist,args,val);
}




}  // namespace mimium