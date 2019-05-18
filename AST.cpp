#include "AST.hpp"

std::shared_ptr<BaseAST> FcallAST::getArgs(int i){
    if(i<Args.size()){
        return Args.at(i);
    }else{
        return NULL;
    }
}