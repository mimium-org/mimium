#pragma once

#include <memory>
#include <unordered_map> 
#include <string>
#include <variant>
#include "ast.hpp"
#include "scheduler.hpp"
//helper type for visiting
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

#include "closure.hpp"

#include "builtin_functions.hpp"

namespace mimium{
class Scheduler; //forward
class Interpreter: public std::enable_shared_from_this<Interpreter> {
    AST_Ptr topast;
    std::shared_ptr<Environment> rootenv;
    std::shared_ptr<Environment> currentenv;
    std::map<std::string,mValue> arguments;
    std::string currentNS;
    std::shared_ptr<Scheduler> sch;

    public:
    Interpreter(){
        rootenv = std::make_shared<Environment>("root",nullptr);
        currentenv = rootenv; // share
    };
    mValue findVariable(std::string str){ //fortest
        auto it = arguments.find(str);
        if(it!=arguments.end()){
            return it->second;
        }else{
            return currentenv->findVariable(str);
        }
    }
    void add_scheduler(){sch = std::make_shared<Scheduler>(this);};
    void start();
    void stop();
    mValue loadAst(AST_Ptr _ast);
    mValue interpretListAst(AST_Ptr ast);
    mValue interpretStatementsAst(AST_Ptr ast);
    mValue interpretTopAst(){return interpretListAst(topast);};
    mValue interpretAssign(AST_Ptr line);
    mValue interpretReturn(AST_Ptr line);
    mValue interpretDeclaration(AST_Ptr expr);


    mValue interpretVariable(AST_Ptr symbol);

    mValue interpretExpr(AST_Ptr expr);
    mValue interpretBinaryExpr(AST_Ptr expr);

    mValue interpretNumber(AST_Ptr num);
    mValue interpretLambda(AST_Ptr expr);

    mValue interpretFcall(AST_Ptr expr);

    mValue interpretArray(AST_Ptr array);
    mValue interpretArrayAccess(AST_Ptr arrayast);


    mValue interpretIf(AST_Ptr expr);

    mValue interpretFor(AST_Ptr expr);

    mValue interpretTime(AST_Ptr expr);

    static double get_as_double(mValue v);
    static std::string to_string(mValue v);
    
    bool assertArgumentsLength(std::vector<AST_Ptr>& args, int length);
};


}


