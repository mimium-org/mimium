#include "closure.hpp"

namespace mimium{
std::string Closure::to_string(){
    std::stringstream ss;
    ss << "Closure:<" << fun->toString() << "> , " << env->getName(); 
    return ss.str();
}

};