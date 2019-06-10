#pragma once 

#include <map>
#include <utility> //pair

#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <list>  
#include <iostream>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

//currently global

static llvm::LLVMContext TheContext;
static auto TheModule =  std::make_unique<llvm::Module>("top", TheContext);
static llvm::IRBuilder<> Builder(TheContext)  ;
static std::map<std::string, llvm::Value *> NamedValues;
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
    virtual std::ostream& to_string(std::ostream &ss) = 0;
    virtual void addAST(AST_Ptr ast){};//for list/argument ast

    virtual llvm::Value *codegen() = 0;


    AST_ID getid(){return id;}
    void set_time(int t){time = t;}
    int get_time(){return time;}
    bool istimeset(){return (time>=0);}
    protected:

    AST_Ptr LogError(const char *Str) {
        std::cerr<< "Error: " << Str << std::endl;
        return nullptr;
    }
    llvm::Value *LogErrorV(const char *Str) {
        LogError(Str);
        return nullptr;
    }
    private:
    int time = -1;

};

class OpAST : public AST,public std::enable_shared_from_this<OpAST>{
    public:
    std::string op;
    int op_id;
    AST_Ptr lhs,rhs;
    
    OpAST(std::string Op,AST_Ptr LHS, AST_Ptr RHS):op(Op),lhs(std::move(LHS)),rhs(std::move(RHS)){
        id=OP;
    }
    OpAST(int Op_id,AST_Ptr LHS, AST_Ptr RHS):op_id(Op_id),lhs(std::move(LHS)),rhs(std::move(RHS)){
        id=OP;
    }

    virtual llvm::Value *codegen() = 0;
    std::ostream& to_string(std::ostream& ss);
    protected:
    auto codegen_pre();

    // static int getop(std::string op){return mimium::op_map[op];}
};
struct AddAST: public OpAST{
    AddAST(AST_Ptr LHS, AST_Ptr RHS): OpAST("+",std::move(LHS) ,std::move(RHS)){};
    llvm::Value *codegen() override;
};
struct SubAST: public OpAST{
    SubAST(AST_Ptr LHS, AST_Ptr RHS): OpAST("-",std::move(LHS) ,std::move(RHS)){};
    llvm::Value *codegen() override;
};
struct MulAST: public OpAST{
    MulAST(AST_Ptr LHS, AST_Ptr RHS): OpAST("*",std::move(LHS) ,std::move(RHS)){};
    llvm::Value *codegen() override;
};
struct DivAST: public OpAST{
    DivAST(AST_Ptr LHS, AST_Ptr RHS): OpAST("/",std::move(LHS) ,std::move(RHS)){};
    llvm::Value *codegen() override;
};
class ListAST : public AST,public std::enable_shared_from_this<ListAST>{
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
    std::ostream& to_string(std::ostream& ss);
    llvm::Value *codegen();

};
class NumberAST :  public AST,public std::enable_shared_from_this<NumberAST>{
    public:
    int val;
    NumberAST(int input): val(input){
        id=NUMBER;
    }
    std::ostream& to_string(std::ostream& ss);
    llvm::Value *codegen();
};

class SymbolAST :  public AST{
    public:
    std::string val;
    SymbolAST(std::string input): val(input){
        id=SYMBOL;
    }
    std::ostream& to_string(std::ostream& ss);
    llvm::Value *codegen();
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
    std::ostream& to_string(std::ostream& ss);
    llvm::Value *codegen();
};

class LambdaAST: public AST{
    public:
    AST_Ptr args;
    AST_Ptr body;
    LambdaAST(AST_Ptr Args, AST_Ptr Body): args(std::move(Args)),body(std::move(Body)){
        id = LAMBDA;
    }
    std::ostream& to_string(std::ostream& ss);
    llvm::Value *codegen();
};

class AssignAST :  public AST{
    public:
    AST_Ptr symbol;
    AST_Ptr expr;
    AssignAST(AST_Ptr Symbol,AST_Ptr Expr): symbol(std::move(Symbol)),expr(std::move(Expr)){
        id=ASSIGN;
    }
    std::ostream& to_string(std::ostream& ss);
    llvm::Value *codegen();
};