#pragma once
#include "environment.hpp"
#include "ast.hpp"
namespace mimium{
struct Closure{
    std::shared_ptr<Environment<mValue>> env;
    LambdaAST& fun;
    Closure(std::shared_ptr<Environment<mValue>> Env,LambdaAST& Fun):env(std::move(Env)),fun(Fun){};

    std::string toString();
};
};