#include "builtin_functions.hpp"

void mimium::builtin::print(char ch){
    std::cout << ch ;
}

void mimium::builtin::print(std::string str){
    std::cout << str ;
}

void mimium::builtin::println(std::string str){
    std::cout << str <<std::endl;
}

bool mimium::builtin::isBuiltin(std::string str){
    return builtin_fntable.count(str)>0;
}