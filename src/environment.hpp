#pragma once
#include <memory>
#include "helper_functions.hpp"
#include "ast.hpp"

//helper type for visiting
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

using mClosure_ptr = std::shared_ptr<mimium::Closure>;
using mValue = std::variant<double,std::shared_ptr<AST>,mClosure_ptr,std::vector<double> >;

namespace mimium{
    class Environment: public std::enable_shared_from_this<Environment>{
    std::map<std::string,mValue,std::less<>> variables;
    std::shared_ptr<Environment> parent;
    std::vector<std::shared_ptr<Environment>> children;
    std::string name;
    public:
    Environment():parent(nullptr),name(""){}
    Environment(std::string Name,std::shared_ptr<Environment> Parent):parent(std::move(Parent)),name(std::move(Name)){
    }
    mValue findVariable(std::string key);
    bool isVariableSet(std::string key);
    void setVariable(std::string key,mValue val);
    void setVariableRaw(std::string key,mValue val);


    auto& getVariables(){return variables;}
    auto getParent(){return parent;}
    std::string getName(){return name;};
    std::shared_ptr<Environment> createNewChild(std::string newname);
    void deleteLastChild();

    static double get_as_double(mValue v) {//duplicating function,,,
    return std::visit(
      overloaded{[](double v) -> double { return v; },
                 [](auto v) -> double {
                   throw std::runtime_error("value is not double");
                   return 0;
                 }},
      v);
};
};
};