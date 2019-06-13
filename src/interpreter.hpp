#pragma once

#include <memory>
#include <unordered_map> 
#include <string>

#include "ast.hpp"

namespace mimium{

class Environment:std::enable_shared_from_this<Environment>{
    std::unordered_map<std::string,std::shared_ptr<AST>> variables;
    std::shared_ptr<Environment> parent;
    std::vector<std::shared_ptr<Environment>> children;
    std::string name;
    public:
    Environment():parent(nullptr),name(""){}
    Environment(std::string Name,std::shared_ptr<Environment> Parent):parent(Parent),name(Name){
    }
    AST_Ptr findVariable(std::string key);
    auto& getVariables(){return variables;}
    std::shared_ptr<Environment> createNewChild(std::string newname);
};

class Interpreter{
    AST_Ptr ast;
    std::shared_ptr<Environment> rootenv;
    std::shared_ptr<Environment> currentenv;
    std::string currentNS;
    bool res;
    public:
    Interpreter():res(false){
        rootenv = std::make_shared<Environment>("root",nullptr);
        currentenv = rootenv; // share
    };
    bool loadAst(AST_Ptr _ast);
    bool interpretTopAst();
    bool interpretAssign(AST_Ptr line);
    bool interpretFdef(AST_Ptr line);
    // bool genEventGraph();
};


}