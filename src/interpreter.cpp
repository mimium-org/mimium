
#include "interpreter.hpp"
void Interpreter::loadAst(AST_Ptr _ast){
    ast = _ast;
}

void Interpreter::interpretAst(){
        switch (ast->getid())
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