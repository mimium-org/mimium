#pragma once

#include <memory>
#include <unordered_map> 
#include <string>
#include <variant>
#include "builtin_functions.hpp"
#include "ast.hpp"
#include "scheduler.hpp"



namespace mimium{
class Environment: public std::enable_shared_from_this<Environment>{
    std::map<std::string,mValue> variables;
    std::shared_ptr<Environment> parent;
    std::vector<std::shared_ptr<Environment>> children;
    std::string name;
    public:
    Environment():parent(nullptr),name(""){}
    Environment(std::string Name,std::shared_ptr<Environment> Parent):parent(Parent),name(Name){
    }
    mValue findVariable(std::string key);
    bool isVariableSet(std::string key);
    void setVariable(std::string key,mValue val);

    auto& getVariables(){return variables;}
    auto getParent(){return parent;}
    std::string getName(){return name;};
    std::shared_ptr<Environment> createNewChild(std::string newname);
};

struct Closure{
    std::shared_ptr<Environment> env;
    std::shared_ptr<LambdaAST> fun;
    Closure(std::shared_ptr<Environment> Env,std::shared_ptr<LambdaAST> Fun):env(Env),fun(Fun){};

    std::string to_string();
};

};

using mClosure_ptr = std::shared_ptr<mimium::Closure>;
using mValue = std::variant<double,std::shared_ptr<AST>,mClosure_ptr>;

namespace mimium{
class Scheduler; //forward
class Interpreter: public std::enable_shared_from_this<Interpreter> {
    AST_Ptr topast;
    std::shared_ptr<Environment> rootenv;
    std::shared_ptr<Environment> currentenv;
    std::map<std::string,AST_Ptr> arguments;
    std::string currentNS;
    std::shared_ptr<Scheduler> sch;

    bool res;
    public:
    Interpreter():res(false){
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

    mValue interpretVariable(AST_Ptr symbol);

    mValue interpretExpr(AST_Ptr expr);
    mValue interpretBinaryExpr(AST_Ptr expr);

    mValue interpretNumber(AST_Ptr num);
    mValue interpretLambda(AST_Ptr expr);

    mValue interpretFcall(AST_Ptr expr);

    static double get_as_double(mValue v);
    static std::string to_string(mValue v);
    
    // bool genEventGraph();
};


}


