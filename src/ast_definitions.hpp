#pragma once 
#include <map>
#include <string>
#include <memory>


enum AST_ID{
    BASE,
    NUMBER,
    FCALL,
    ASSIGN,
    FDEF,
    ARRAYINIT,
    LAMBDA,
    OP
};


namespace mimium{
    const std::map<std::string,AST_ID> ast_id = {
        {"fcall",FCALL},
        {"define",ASSIGN},
        {"fdef",FDEF},
        {"array",ARRAYINIT},
        {"lambda",LAMBDA}
    };
    static AST_ID str_to_id(std::string str){
        auto id = mimium::ast_id.find(str);
                if(id!=std::end(mimium::ast_id)){
                        return id->second;
                }else{
                        return NUMBER;
                }
    };
};

class AST{
    public:
    AST_ID id;
};
using AST_Ptr = std::shared_ptr<AST>;

class OpAST : AST{
    public:
    AST_Ptr lhs,rhs;
    std::string op;
    OpAST(std::string& OPERATOR,AST_Ptr LHS, AST_Ptr RHS){
        id=OP;
        lhs = LHS;
        rhs = RHS;
        op = OPERATOR;
    }
};