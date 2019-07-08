#pragma once 

#include <string>
#include <map>
#include <iostream>
#include <memory>
#include <variant>
#include "ast.hpp"
#include "interpreter.hpp"
#include "mididriver.hpp"
using mValue = std::variant<double,std::shared_ptr<AST>,mClosure_ptr,std::vector<double> >;
// template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace mimium{
    class Interpreter;//forward?
    namespace builtin{

    mValue print(std::shared_ptr<ArgumentsAST> argast,Interpreter* interpreter);
    mValue println(std::shared_ptr<ArgumentsAST>  argast,Interpreter* interpreter);

    bool isBuiltin(std::string str);

    static Mididriver midi; //todo: need to split extensions nicely
    mValue setMidiOut(std::shared_ptr<ArgumentsAST>,Interpreter* interpreter);
    mValue sendMidiMessage(std::shared_ptr<ArgumentsAST>,Interpreter* interpreter);

    const static std::map<std::string,mValue(*)(std::shared_ptr<ArgumentsAST>,Interpreter*)> builtin_fntable = {
        {"printchar" ,&print},
        {"print", &print},
        {"println",&println},
        {"setMidiOut",&setMidiOut},
        {"sendMidiMessage",&sendMidiMessage}
    };
}
}