#pragma once 

#include <string>
#include <map>
#include <cmath>
#include <iostream>
#include <memory>
#include <variant>
#include "ast.hpp"
#include "interpreter_visitor.hpp"
#include "mididriver.hpp"

namespace mimium{
    class InterpreterVisitor;//forward?
    using mmmfn = mValue(*)(std::shared_ptr<ArgumentsAST>,InterpreterVisitor*);

    class Builtin{
        public:
        static mValue print(std::shared_ptr<ArgumentsAST>argast ,InterpreterVisitor* interpreter );
        static mValue println(std::shared_ptr<ArgumentsAST>argast ,InterpreterVisitor* interpreter );
        static mValue setMidiOut(std::shared_ptr<ArgumentsAST>argast ,InterpreterVisitor* interpreter );
        static mValue setVirtualMidiOut(std::shared_ptr<ArgumentsAST>argast ,InterpreterVisitor* interpreter );
        static mValue sendMidiMessage(std::shared_ptr<ArgumentsAST>argast ,InterpreterVisitor* interpreter );
        static mValue cmath(std::function<double(double)> fn,
                      std::shared_ptr<ArgumentsAST> argast,
                      InterpreterVisitor *interpreter);

        static mValue sin(std::shared_ptr<ArgumentsAST>,InterpreterVisitor* interpreter);
        static mValue cos(std::shared_ptr<ArgumentsAST>,InterpreterVisitor* interpreter);

        // const static mmmfn createMathFn(std::function<double(double)> fn, std::shared_ptr<ArgumentsAST>argast ,InterpreterVisitor interpreter );

        const static bool isBuiltin(std::string str);
        const static std::map<std::string,mmmfn> builtin_fntable; 
        private:
        Builtin()=default;
        ~Builtin()=default;
        static std::vector<unsigned char> midiSendVisitor(mValue v,InterpreterVisitor* interpreter);
    }; // namespace Builtin
} //namespace mimium