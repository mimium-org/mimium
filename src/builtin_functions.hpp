#pragma once 

#include <string>
#include <map>
#include <iostream>
#include <variant>
#include "ast.hpp"
#include "interpreter.hpp"
using mValue = std::variant<double,std::shared_ptr<AST>,mClosure_ptr,std::vector<double> >;
// template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace mimium::builtin{

    mValue print(mValue str);
    mValue println(mValue str);
    mValue cons(mValue val1,mValue val2);
    mValue car(mValue val);
    mValue cdr(mValue val);
    bool isBuiltin(std::string str);
    const static std::map<std::string,mValue(*)(mValue)> builtin_fntable = {
        {"printchar" ,&print},
        {"print", &print},
        {"println",&println}
    };
}