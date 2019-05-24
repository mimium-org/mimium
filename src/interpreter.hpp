#pragma once

#include <memory>
#include "s-exp.hpp"

using S_Ptr = std::shared_ptr<S_Expr>;

class Interpreter{
    S_Ptr ast;
    public:
    void loadAst(S_Ptr _ast);
    void interpretAst();
    void genEventGraph();
};

