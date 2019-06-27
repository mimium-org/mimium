#pragma once 

#include <string>
#include <map>
#include <iostream>
#include <variant>
#include "ast.hpp"
#include "interpreter.hpp"
using mValue = std::variant<double,std::shared_ptr<AST>,mClosure_ptr>;
// template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace mimium::builtin{

    void print(mValue str);
    void println(mValue str);
    bool isBuiltin(std::string str);
    const static std::map<std::string,void(*)(mValue)> builtin_fntable = {
        {"printchar" ,&print},
        {"print", &print},
        {"println",&println}
    };
}