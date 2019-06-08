#pragma once 
#include <map>
#include <string>
#include <sstream>
#include <memory>
#include <vector>
 #include <iostream>

enum AST_ID{
    BASE,
    NUMBER,
    FCALL,
    ASSIGN,
    FDEF,
    ARRAYINIT,
    LAMBDA,
    OP,
    LIST
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
class AST;
using AST_Ptr = std::unique_ptr<AST>;

class AST{
    public:
    AST_ID id;
    virtual ~AST()=default;
    virtual std::string to_string() = 0;
    virtual void addAST(AST_Ptr ast){};//for list ast

    AST_ID getid(){return id;}
    void set_time(int t){time = t;}
    int get_time(){return time;}
    bool istimeset(){return (time>=0);}
    int time = -1;
    private:
    
};

class OpAST : public AST,public std::enable_shared_from_this<OpAST>{
    public:
    std::string op;
    AST_Ptr lhs,rhs;
    
    OpAST(std::string& Op,AST_Ptr LHS, AST_Ptr RHS):op(Op),lhs(std::move(LHS)),rhs(std::move(RHS)){
        id=OP;
    }
    std::string to_string();
};
class ListAST : AST{
    public:
    std::vector<AST_Ptr> asts;
    ListAST(std::vector<AST_Ptr> _asts):asts(std::move(_asts)){
        id = LIST;
    }
    void addAST(AST_Ptr ast) {
        asts.push_back(std::move(ast));
    }

    std::string to_string();
};
class NumberAST :  public AST,public std::enable_shared_from_this<NumberAST>{
    public:
    int val;
    NumberAST(int input): val(input){
        id=NUMBER;
    }
    std::string to_string();
};