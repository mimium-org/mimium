#pragma once 

#include <string>
#include <map>
#include <iostream>
#include <memory>
#include <variant>
#include "ast.hpp"
#include "interpreter.hpp"
using mValue = std::variant<double,std::shared_ptr<AST>,mClosure_ptr,std::vector<double> >;
// template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace mimium{
    class Interpreter;//forward?
    namespace builtin{

    mValue print(std::shared_ptr<ArgumentsAST> argast,mimium::Interpreter* interpreter);
    mValue println(std::shared_ptr<ArgumentsAST>  argast,mimium::Interpreter* interpreter);

    bool isBuiltin(std::string str);
    const static std::map<std::string,mValue(*)(std::shared_ptr<ArgumentsAST>,mimium::Interpreter*)> builtin_fntable = {
        {"printchar" ,&print},
        {"print", &print},
        {"println",&println}
    };
}
}