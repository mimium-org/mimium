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
void NumberInst::closureConvert(std::deque<std::string>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
  cc->env->setVariableRaw(lv_name,std::to_string(val));
}

std::string SymbolInst::toString() { return lv_name + " = " + val ; }
void SymbolInst::closureConvert(std::deque<std::string>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
  //do nothing
}

std::string RefInst::toString() { return lv_name + " = ref " + val ; }
void RefInst::closureConvert(std::deque<std::string>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
  cc->env->setVariableRaw(lv_name,val);
}

std::string TimeInst::toString() { return lv_name + " = " + val + +"@" + time; }
void TimeInst::closureConvert(std::deque<std::string>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
  gatherFV_raw(fvlist,cc->env,val);
  gatherFV_raw(fvlist,cc->env,time);
  cc->env->setVariableRaw(lv_name,val);
}
std::string OpInst::toString() {
  return lv_name + " = " + lhs + op + rhs;

}
void OpInst::closureConvert(std::deque<std::string>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
  gatherFV_raw(fvlist,cc->env,lhs);
  gatherFV_raw(fvlist,cc->env,rhs);
  cc->env->setVariableRaw(lv_name,"some_op");

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
void FunInst::closureConvert(std::deque<std::string>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
  auto tmpenv = cc->env;
  cc->env =  cc->env->createNewChild(lv_name);
  for(auto& a : this->args){
    cc->env->setVariableRaw(a,"arg");
  }
  for(auto cit = body->instructions.begin(),end = body->instructions.end() ;cit!=end; cit++){
    auto& childinst = *cit;
    std::visit([&](auto c){c->closureConvert(this->freevariables,cc,this->body,cit);} ,childinst);//recursively visit;
  }
  if(this->freevariables.size()<=0){
    cc->known_functions[lv_name] = shared_from_this();
  }else{
    std::string newname = lv_name+"$cls"+ std::to_string(cc->capturecount++);
    auto makecls = std::make_shared<MakeClosureInst>(newname,lv_name,this->freevariables);
    mir->instructions.insert(it,std::move(makecls));

  }
  cc->env = tmpenv;
  cc->env->setVariableRaw(lv_name,"some_fun");
  //post process?????
}

void FunInst::moveFunToTop(std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
  std::cout << lv_name <<std::endl;
  auto& tinsts = cc->toplevel->instructions;
  auto cit =  body->instructions.begin();
  while(cit != body->instructions.end()){
    auto& childinst = *cit;
    std::visit(overloaded{
      [&](std::shared_ptr<FunInst> c){c->moveFunToTop(cc,this->body,cit);cit++;},
      [&cit](auto c){cit++;}
      } ,childinst);//recursively visit;
  }
  if(cc->toplevel!=mir){
  tinsts.insert(tinsts.begin(),shared_from_this());
  }
  this->body->instructions.remove_if([](Instructions v){
    return std::visit([](auto v)->bool{return v->isFunction();},v);}
    );
}

std::string MakeClosureInst::toString(){
std::string s;
  s += lv_name + " = makeclosure " + fname + " " + join(captures," , ") ;
  // s += body->toString();
  return s;
}
void MakeClosureInst::closureConvert(std::deque<std::string>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
//do nothing
}
std::string FcallInst::toString(){
    std::string s;
    return lv_name + " = app"+fcalltype_str[ftype] + " " + fname +" "+ join(args," , ")  ;
}
void FcallInst::closureConvert(std::deque<std::string>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
  bool isknown = cc->known_functions.count(this->fname)>0;
  if(isknown){
    this->ftype = DIRECT;
  }
  for(auto& a : this->args){
    gatherFV_raw(fvlist,cc->env,a);
  }
  cc->env->setVariableRaw(lv_name,"some_fun");
}

std::string ArrayInst::toString(){
    return lv_name + " = array " + name + " " + join(args," , ");
}

void ArrayInst::closureConvert(std::deque<std::string>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
  for(auto& a : this->args){
    gatherFV_raw(fvlist,cc->env,a);
  }
  cc->env->setVariableRaw(lv_name,"some_array");
}

std::string ArrayAccessInst::toString(){
    return lv_name + " = arrayaccess " + name + " " + index;
}
void ArrayAccessInst::closureConvert(std::deque<std::string>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
    gatherFV_raw(fvlist,cc->env,name);
    gatherFV_raw(fvlist,cc->env,index);
    cc->env->setVariableRaw(lv_name,"some_access");

}

std::string IfInst::toString(){
  std::string s;
  s+= lv_name + " = if " + cond +"\n";
  s+= thenblock->toString();
  s+= elseblock->toString();
  return s;
}
void IfInst::closureConvert(std::deque<std::string>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
    gatherFV_raw(fvlist,cc->env,cond);
    for(auto cit = thenblock->instructions.begin(),end =thenblock->instructions.end();cit!=end; cit++){
      auto& theninst = *cit;
      std::visit([&](auto c){c->closureConvert(fvlist,cc,mir,cit);},theninst);
    }
    for(auto cit = elseblock->instructions.begin(),end =elseblock->instructions.end();cit!=end; cit++){
      auto& elseinst = *cit;
      std::visit([&](auto c){c->closureConvert(fvlist,cc,mir,cit);},elseinst);
    }
}
std::string ReturnInst::toString(){
    return lv_name + " = return " + val ;
}
void ReturnInst::closureConvert(std::deque<std::string>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
      gatherFV_raw(fvlist,cc->env,val);
      // std::visit( overloaded{
      //   [](types::Function f){},
      //   [](auto c){}
      // },this->type);
}




}  // namespace mimium