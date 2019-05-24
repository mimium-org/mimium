#include "interpreter.hpp"

using S_Ptr = std::shared_ptr<S_Expr>;


void Interpreter::loadAst(S_Ptr _ast){
    ast = _ast;
}

void Interpreter::interpretAst(){
        switch (ast->get_head())
    {
    case /* constant-expression */:
        /* code */
        break;
    
    default:
        break;
    }
    
}