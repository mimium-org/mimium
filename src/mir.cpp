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
    str += inst->toString() + "\n";
  }
  indent_level--;
  return str;
}

std::string NumberInst::toString() {
  return lv_name + " = " + std::to_string(val);
}
std::string SymbolInst::toString() { return lv_name + " = " + val ; }
std::string TimeInst::toString() { return lv_name + " = " + val + +"@" + time; }

std::string OpInst::toString() {
  return lv_name + " = " + lhs + op + rhs;
}
std::string FunInst::toString() {
  std::string s;
  s += lv_name + " = fun " + join(args," , ")+"\n";
  MIRblock::indent_level++;
  s += body->toString();
  MIRblock::indent_level--;

  return s;
}
std::string MakeClosureInst::toString(){
std::string s;
  s += lv_name + " = makeclosure " + join(captures," , ") ;
  // s += body->toString();
  return s;
}
std::string FcallInst::toString(){
    std::string s;
    return lv_name + " = app"+fcalltype_str[type] + " " + fname +" "+ join(args," , ") ;
}

std::string ArrayInst::toString(){
    return lv_name + " = array " + name + " " + join(args," , ");
}
std::string ArrayAccessInst::toString(){
    return lv_name + " = arrayaccess " + name + " " + index;
}
std::string IfInst::toString(){
  std::string s;
  s+= lv_name + " = if " + cond +"\n";
  MIRblock::indent_level++;
  s+= thenblock->toString();
  s+= elseblock->toString();
    MIRblock::indent_level--;
    return s;
}
std::string ReturnInst::toString(){
    return lv_name + " = return " + val ;
}




}  // namespace mimium