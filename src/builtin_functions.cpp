#include "builtin_functions.hpp"


void mimium::builtin::print(mValue str){
    std::visit(overloaded{
        [](double s){std::cout << s;},
        [](mClosure_ptr c){std::cout <<  c->to_string() ;},
        [](auto c){std::cout << "print function not implemented";}
    },str);
}

void mimium::builtin::println(mValue str){
    std::visit(overloaded{
        [](double s){std::cout << s;},
        [](mClosure_ptr c){std::cout << c->to_string();},
        [](auto c){std::cout << "print function not implemented";}
    },str);}

bool mimium::builtin::isBuiltin(std::string str){
    return builtin_fntable.count(str)>0;
}