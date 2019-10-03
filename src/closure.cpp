#include "closure.hpp"

namespace mimium{
std::string Closure::toString(){
    std::stringstream ss;
    ss << "Closure:<" << fun->toString() << "> , " << env->getName(); 
    return ss.str();
}

};