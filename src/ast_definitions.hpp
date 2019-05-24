#pragma once 
#include <map>
// using S_Ptr = std::shared_ptr<S_Expr>;

enum AST_ID{
    NUMBER,
    FCALL,
    ASSIGN,
    FDEF,
    ARRAYINIT,
    LAMBDA
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