#pragma once

#include <memory>
#include <unordered_map> 

#include "ast.hpp"

class Interpreter{
    AST_Ptr ast;
    std::unordered_map<std::string,AST_Ptr> environment;
    public:
    void loadAst(AST_Ptr _ast);
    void interpretAst();
    void genEventGraph();
};

