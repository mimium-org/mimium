#pragma once
#include <memory>
#include "helper_functions.hpp"
#include "ast.hpp"

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
    Environment(std::string Name,std::shared_ptr<Environment> Parent):parent(Parent),name(Name){
    }
    mValue findVariable(std::string key);
    bool isVariableSet(std::string key);
    void setVariable(std::string key,mValue val);

    auto& getVariables(){return variables;}
    auto getParent(){return parent;}
    std::string getName(){return name;};
    std::shared_ptr<Environment> createNewChild(std::string newname);
    void deleteLastChild();
};
};