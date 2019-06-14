#pragma once 

#include <map>
#include <utility> //pair

#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <list>  
#include <iostream>
#include <variant>

struct Closure; // forward

using mValue = std::variant<double,std::shared_ptr<Closure>>;



enum AST_ID{
    BASE,
    NUMBER,
    SYMBOL,
    ARG,
    ARGS,
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
    // static AST_ID str_to_id(std::string str){
    //     auto id = mimium::ast_id.find(str);
    //             if(id!=std::end(mimium::ast_id)){
    //                     return id->second;
    //             }else{
    //                     return NUMBER;
    //             }
    // };
};
class AST;
using AST_Ptr = std::shared_ptr<AST>;

class AST{
    public:
    AST_ID id;
    virtual ~AST()=default;
    virtual std::ostream& to_string(std::ostream &ss) = 0;
    virtual void addAST(AST_Ptr ast){};//for list/argument ast



    AST_ID getid(){return id;}
    void set_time(int t){time = t;}
    int get_time(){return time;}
    bool istimeset(){return (time>=0);}
    protected:

    AST_Ptr LogError(const char *Str) {
        std::cerr<< "Error: " << Str << std::endl;
        return nullptr;
    }

    private:
    int time = -1;

};
enum OP_ID{
    ADD,
    SUB,
    MUL,
    DIV
};
static std::map<std::string, OP_ID> optable={
        {"+",ADD},
        {"-",SUB},
        {"*",MUL},
        {"/",DIV}
        };

class OpAST : public AST{
    public:
    std::string op;
    OP_ID op_id;
    AST_Ptr lhs,rhs;
    
    OpAST(std::string Op,AST_Ptr LHS, AST_Ptr RHS);
    OpAST(OP_ID Op_id,AST_Ptr LHS, AST_Ptr RHS):op_id(Op_id),lhs(std::move(LHS)),rhs(std::move(RHS)){
        id=OP;
    }

    std::ostream& to_string(std::ostream& ss);
    OP_ID getOpId(){return op_id;};
    protected:

    // static int getop(std::string op){return mimium::op_map[op];}
};
struct AddAST: public OpAST{
    AddAST(AST_Ptr LHS, AST_Ptr RHS): OpAST("+",std::move(LHS) ,std::move(RHS)){};
};
struct SubAST: public OpAST{
    SubAST(AST_Ptr LHS, AST_Ptr RHS): OpAST("-",std::move(LHS) ,std::move(RHS)){};
};
struct MulAST: public OpAST{
    MulAST(AST_Ptr LHS, AST_Ptr RHS): OpAST("*",std::move(LHS) ,std::move(RHS)){};
};
struct DivAST: public OpAST{
    DivAST(AST_Ptr LHS, AST_Ptr RHS): OpAST("/",std::move(LHS) ,std::move(RHS)){};
};
class ListAST : public AST{
    public:
    std::list<AST_Ptr> asts;
    ListAST(){ //empty constructor
        id=LIST;
    }

    ListAST(AST_Ptr _asts){
        asts.push_front(std::move(_asts));
        id = LIST;
    }
    void addAST(AST_Ptr ast) {
        asts.push_back(std::move(ast));
    }
    std::list<AST_Ptr>& getlist(){return asts;};
    std::ostream& to_string(std::ostream& ss);

};
class NumberAST :  public AST{
    public:
    mValue val;
    NumberAST(mValue input): val(input){
        id=NUMBER;
    }
    mValue& getVal(){return val;};

    std::ostream& to_string(std::ostream& ss);
};

class SymbolAST :  public AST{
    public:
    std::string val;
    SymbolAST(std::string input): val(input){
        id=SYMBOL;
    }
    std::string& getVal(){return val;};

    std::ostream& to_string(std::ostream& ss);
};

class ArgumentsAST : public AST{
    public:
    std::list<AST_Ptr> args;

    ArgumentsAST(AST_Ptr arg){
        args.push_front(std::move(arg));
        id=ARGS;
    }
    void addAST(AST_Ptr arg){
        args.push_front(std::move(arg));
    };
    auto& getArgs(){return args;}
    std::ostream& to_string(std::ostream& ss);
};

class LambdaAST: public AST{
    public:
    AST_Ptr args;
    AST_Ptr body;
    LambdaAST(AST_Ptr Args, AST_Ptr Body): args(std::move(Args)),body(std::move(Body)){
        id = LAMBDA;
    }
    auto getArgs(){return std::dynamic_pointer_cast<ArgumentsAST>(args);};
    AST_Ptr getBody(){return std::move(body);};
    std::ostream& to_string(std::ostream& ss);
};

class AssignAST :  public AST{
    public:
    AST_Ptr symbol;
    AST_Ptr expr;
    AssignAST(AST_Ptr Symbol,AST_Ptr Expr): symbol(std::move(Symbol)),expr(std::move(Expr)){
        id=ASSIGN;
    }
    auto getName(){return std::dynamic_pointer_cast<SymbolAST>(std::move(symbol));};
    AST_Ptr getBody(){return std::move(expr);};

    std::ostream& to_string(std::ostream& ss);
};

class FcallAST: public AST{
    public:
    AST_Ptr fname;
    AST_Ptr args;
    FcallAST(AST_Ptr Fname, AST_Ptr Args): fname(std::move(Fname)),args(std::move(Args)){
        id = FCALL;
    }
    auto getArgs(){return std::dynamic_pointer_cast<ArgumentsAST>(args); };
    auto getFname(){return std::dynamic_pointer_cast<SymbolAST>(fname);};
    std::ostream& to_string(std::ostream& ss);
};