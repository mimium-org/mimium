#include "closure.hpp"

namespace mimium{
std::string Closure::to_string(){
    std::stringstream ss;
    ss << "Closure:<";
    fun->to_string(ss);
    ss << "> , " << env->getName(); 
    return ss.str();
}

};