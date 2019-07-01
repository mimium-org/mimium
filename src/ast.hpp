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



enum AST_ID{
    BASE,
    NUMBER,
    SYMBOL,
    ARG,
    ARGS,
    ARRAY,
    FCALL,
    ASSIGN,
    FDEF,
    RETURN,
    ARRAYINIT,
    LAMBDA,
    OP,
    IF,
    LIST,
    TIME

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
class AST;//forward
namespace mimium{
struct Closure; //forward
};
using mClosure_ptr = std::shared_ptr<mimium::Closure>;

using AST_Ptr = std::shared_ptr<AST>;
using mValue = std::variant<double,std::shared_ptr<AST>,mClosure_ptr,std::vector<double> >;



class AST{
    public:
    AST_ID id;
    virtual ~AST()=default;
    virtual std::ostream& to_string(std::ostream &ss) = 0;
    virtual void addAST(AST_Ptr ast){};//for list/argument ast



    AST_ID getid(){return id;}
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
    DIV,
    EXP,
    MOD,
    AND,
    OR,
    BITAND,
    BITOR,
    LE,
    GE,
    LT,
    GT,
    LSHIFT,
    RSHIFT
};
static std::map<std::string, OP_ID> optable={
        {"+",ADD},
        {"-",SUB},
        {"*",MUL},
        {"/",DIV},
        {"^",EXP},
        {"%",MOD},
        {"&",AND},
        {"|",OR},
        {"&&",BITAND},
        {"||",BITOR},
        {"<",LT},
        {">",GT},
        {"<=",LE},
        {">=",GE},
        {"<<",LSHIFT},
        {">>",RSHIFT}
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

class ListAST : public AST{
    public:
    std::vector<AST_Ptr> asts;
    ListAST(){ //empty constructor
        id=LIST;
    }

    ListAST(AST_Ptr _asts){
        asts.push_back(std::move(_asts)); //push_front
        id = LIST;
    }
    void addAST(AST_Ptr ast) {
        asts.insert(asts.begin(),std::move(ast));
    }
    std::vector<AST_Ptr>& getlist(){return asts;};
    std::ostream& to_string(std::ostream& ss);

};
class NumberAST :  public AST{
    public:
    double val;
    NumberAST(double input){
        val=input;
        id=NUMBER;
    }
    double getVal(){return val;};

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

class AbstractListAST : public AST{
    public:
    std::vector<AST_Ptr> elements;

    AbstractListAST(AST_Ptr arg){
        elements.insert(elements.begin(),std::move(arg));
    }
    void addAST(AST_Ptr arg){
        elements.insert(elements.begin(),std::move(arg));
    };
    auto& getElements(){return elements;}
    std::ostream& to_string(std::ostream& ss);
};

class ArgumentsAST: public AbstractListAST{
    public:
     ArgumentsAST(AST_Ptr arg):AbstractListAST(std::move(arg)){
            id=ARGS;
     };
};
class ArrayAST: public AbstractListAST{
    public:
    ArrayAST(AST_Ptr arg):AbstractListAST(std::move(arg)){
            id=ARRAY;
     };
};
class LambdaAST: public AST{
    public:
    AST_Ptr args;
    AST_Ptr body; //statements
    LambdaAST(AST_Ptr Args, AST_Ptr Body): args(std::move(Args)),body(std::move(Body)){
        id = LAMBDA;
    }
    auto getArgs(){return std::dynamic_pointer_cast<ArgumentsAST>(args);};
    AST_Ptr getBody(){return body;};
    std::ostream& to_string(std::ostream& ss);
};

class AssignAST :  public AST{
    public:
    AST_Ptr symbol;
    AST_Ptr expr;
    AssignAST(AST_Ptr Symbol,AST_Ptr Expr): symbol(std::move(Symbol)),expr(std::move(Expr)){
        id=ASSIGN;
    }
    auto getName(){return std::dynamic_pointer_cast<SymbolAST>(symbol);};
    AST_Ptr getBody(){return expr;};

    std::ostream& to_string(std::ostream& ss);
};

// class FdefAST :  public AST{
//     public:
//     AST_Ptr fname;
//     AST_Ptr arguments;
//     AST_Ptr statements;
//     AssignAST(AST_Ptr Fname,AST_Ptr Arguments,AST_Ptr Statements): fname(std::move(Fname)),arguments(std::move(Arguments)),statements(std::move(Statements)){
//         id=FDEF;
//     }
//     auto getFname(){return std::dynamic_pointer_cast<SymbolAST>(fname);};
//     auto getArguments(){return arguments;};
//     auto getFbody(){return statements;};

//     std::ostream& to_string(std::ostream& ss);
// };

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
class ReturnAST: public AST{
    public:
    AST_Ptr expr;
    ReturnAST(AST_Ptr Expr):expr(std::move(Expr)){
        id = RETURN;
    }
    auto getExpr(){return expr;}
    std::ostream& to_string(std::ostream& ss);
};
class IfAST: public AST{
    public:
    AST_Ptr condition,thenstatement,elsestatement;
    IfAST(AST_Ptr Condition,AST_Ptr Thenstatement,AST_Ptr Elsestatement):condition(std::move(Condition)),thenstatement(std::move(Thenstatement)),elsestatement(std::move(Elsestatement)){
        id = IF;
    }
    auto getCond(){return condition;}
    auto getThen(){return thenstatement;}
    auto getElse(){return elsestatement;}
    std::ostream& to_string(std::ostream& ss);
};

class TimeAST : public AST{
    public:
    AST_Ptr expr;
    AST_Ptr time;
    TimeAST(AST_Ptr Expr,AST_Ptr Time):expr(std::move(Expr)),time(std::move(Time)){
        id = TIME;
    }
    auto getTime(){return time;}
    auto getExpr(){return expr;}
    std::ostream& to_string(std::ostream& ss);
};