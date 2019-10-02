#include "interpreter_visitor.hpp"


namespace mimium{
    mValue InterpreterVisitor::visit_assign(AssignAST* ast){
            std::string varname = ast->getName()->getVal();//assuming name as symbolast
            auto body = ast->getBody();

            if (body) {
                mValue res = body->accept<mValue>(this);
                itp->getCurrentEnv()->setVariable(varname, res);  // share
                Logger::debug_log(
                    "Variable " + varname + " : " + Interpreter::to_string(res),
                    Logger::DEBUG);
                return res;  // for print
            } else {
                throw std::runtime_error("expression not resolved");
            }
    }
}//namespace mimium