#include "mir.hpp"

namespace mimium {
static std::string join(std::deque<std::string>& vec,std::string delim){
    std::string s;
    for (auto& elem : vec) {
    s += elem;
    auto endstr =vec.back();
    if (elem !=  endstr) s += delim;
  }
  return s;
};
static std::string join(std::deque<TypedVal>& vec,std::string delim){
    std::string s;
    for (auto& elem : vec) {
    s += elem.name;
    auto endstr =vec.back();
    if (elem.name !=  endstr.name) s += delim;
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
void MIRinstruction::gatherFV_raw(std::deque<TypedVal>& fvlist ,std::shared_ptr<Environment> env,TypeEnv& typeenv,std::string& str){
    if(isFreeVariable(env,str)){
      TypedVal tv = {typeenv.env.find(str)->second ,str};
    fvlist.push_back(tv);
    str = "fv_" + str;
  }
}

std::string NumberInst::toString() {
  return lv_name + " = " + std::to_string(val);
}
void NumberInst::closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
  cc->env->setVariableRaw(lv_name,std::to_string(val));
}

std::string SymbolInst::toString() { return lv_name + " = " + val ; }
void SymbolInst::closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
  //do nothing
}

std::string RefInst::toString() { return lv_name + " = ref " + val ; }
void RefInst::closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
  cc->env->setVariableRaw(lv_name,val);
}

std::string TimeInst::toString() { return lv_name + " = " + val + +"@" + time; }
void TimeInst::closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
  gatherFV_raw(fvlist,cc->env,cc->typeenv,val);
  gatherFV_raw(fvlist,cc->env,cc->typeenv,time);
  cc->env->setVariableRaw(lv_name,val);
}
std::string OpInst::toString() {
  return lv_name + " = " + lhs + op + rhs;

}
void OpInst::closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
  gatherFV_raw(fvlist,cc->env,cc->typeenv,lhs);
  gatherFV_raw(fvlist,cc->env,cc->typeenv,rhs);
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
void FunInst::closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
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
    auto fvtype = getFvType(this->freevariables);
    std::string newname = lv_name+"$cls";//+ std::to_string(cc->capturecount++);
    auto makecls = std::make_shared<MakeClosureInst>(newname,lv_name,this->freevariables,fvtype);
    mir->instructions.insert(++it,std::move(makecls));
    types::Function newtype = std::get<recursive_wrapper<types::Function>>(this->type);
    newtype.arg_types.push_back(fvtype);
    this->type = newtype;

  }
  cc->env = tmpenv;
  cc->env->setVariableRaw(lv_name,"some_fun");
  //post process?????
}

types::Struct FunInst::getFvType(std::deque<TypedVal>& fvlist){
  std::vector<types::Value> v;
  for(auto& a:fvlist){
    v.push_back(a.type);
  }
  return types::Struct(v);
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
void MakeClosureInst::closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
//do nothing
}
std::string FcallInst::toString(){
    std::string s;
    return lv_name + " = app"+fcalltype_str[ftype] + " " + fname +" "+ join(args," , ")  ;
}
void FcallInst::closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
  bool isknown = cc->known_functions.count(this->fname)>0;
  if(isknown){
    this->ftype = DIRECT;
  }
  for(auto& a : this->args){
    gatherFV_raw(fvlist,cc->env,cc->typeenv,a);
  }
  cc->env->setVariableRaw(lv_name,"some_fun");
}

std::string ArrayInst::toString(){
    return lv_name + " = array " + name + " " + join(args," , ");
}

void ArrayInst::closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
  for(auto& a : this->args){
    gatherFV_raw(fvlist,cc->env,cc->typeenv,a);
  }
  cc->env->setVariableRaw(lv_name,"some_array");
}

std::string ArrayAccessInst::toString(){
    return lv_name + " = arrayaccess " + name + " " + index;
}
void ArrayAccessInst::closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
    gatherFV_raw(fvlist,cc->env,cc->typeenv,name);
    gatherFV_raw(fvlist,cc->env,cc->typeenv,index);
    cc->env->setVariableRaw(lv_name,"some_access");

}

std::string IfInst::toString(){
  std::string s;
  s+= lv_name + " = if " + cond +"\n";
  s+= thenblock->toString();
  s+= elseblock->toString();
  return s;
}
void IfInst::closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
    gatherFV_raw(fvlist,cc->env,cc->typeenv,cond);
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
void ReturnInst::closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){
      gatherFV_raw(fvlist,cc->env,cc->typeenv,val);
      // std::visit( overloaded{
      //   [](types::Function f){},
      //   [](auto c){}
      // },this->type);
}




}  // namespace mimium