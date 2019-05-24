#pragma once

#include <memory>
#include <unordered_map> 

#include "s-exp.hpp"

using S_Ptr = std::shared_ptr<S_Expr>;

class Interpreter{
    S_Ptr ast;
    std::unordered_map<std::string,S_Ptr> environment;
    public:
    void loadAst(S_Ptr _ast);
    void interpretAst();
    void genEventGraph();
};

