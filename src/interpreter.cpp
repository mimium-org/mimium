#include "interpreter.hpp"

// using S_Ptr = std::shared_ptr<S_Expr>;


void Interpreter::loadAst(S_Ptr _ast){
    ast = _ast;
}

void Interpreter::interpretAst(){
        switch (mimium::str_to_id(ast->get_head()->to_string()))
    {
    case ASSIGN:
        break;
    case FCALL:

        break;
    case FDEF:
        break;
    case ARRAYINIT:
        break;
    case LAMBDA:
        break;

    default:
        break;
    }
    
}