#pragma once 

#include <string>
#include <map>
#include <cmath>
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
    using mmmfn = mValue(*)(std::shared_ptr<ArgumentsAST>,Interpreter*);

    class Builtin{
        public:
        static mValue print(std::shared_ptr<ArgumentsAST>argast ,Interpreter* interpreter );
        static mValue println(std::shared_ptr<ArgumentsAST>argast ,Interpreter* interpreter );
        static mValue setMidiOut(std::shared_ptr<ArgumentsAST>argast ,Interpreter* interpreter );
        static mValue setVirtualMidiOut(std::shared_ptr<ArgumentsAST>argast ,Interpreter* interpreter );
        static mValue sendMidiMessage(std::shared_ptr<ArgumentsAST>argast ,Interpreter* interpreter );
        static mValue cmath(std::function<double(double)> fn,
                      std::shared_ptr<ArgumentsAST> argast,
                      Interpreter *interpreter);

        static mValue sin(std::shared_ptr<ArgumentsAST>,Interpreter*);
        static mValue cos(std::shared_ptr<ArgumentsAST>,Interpreter*);

        // const static mmmfn createMathFn(std::function<double(double)> fn, std::shared_ptr<ArgumentsAST>argast ,Interpreter* interpreter );

        const static bool isBuiltin(std::string str);
        const static std::map<std::string,mmmfn> builtin_fntable; 
        private:
        Builtin(){}
        ~Builtin(){}

    }; // namespace Builtin
} //namespace mimium