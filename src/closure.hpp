#pragma once
#include "environment.hpp"
#include "ast.hpp"
namespace mimium{
struct Closure{
    std::shared_ptr<Environment> env;
    std::shared_ptr<LambdaAST> fun;
    Closure(std::shared_ptr<Environment> Env,std::shared_ptr<LambdaAST> Fun):env(std::move(Env)),fun(std::move(Fun)){};

    std::string toString();
};
};