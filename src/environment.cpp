#include "environment.hpp"
namespace mimium{
bool Environment::isVariableSet(std::string key){
    if(variables.size()>0 && variables.count(key)>0){//search dictionary
        return true;
    }else if(parent !=nullptr){
        return parent->isVariableSet(key); //search recursively
    }else{
        return false;
    }
}

mValue Environment::findVariable(std::string key){
    if(variables.size()>0 && variables.count(key)>0){//search dictionary
        return variables.at(key);
    }else if(parent !=nullptr){
        return parent->findVariable(key); //search recursively
    }else{
        std::cerr << "Variable" << key << "not found" << std::endl;
    
        return 0;
    }
}

void Environment::setVariable(std::string key,mValue val){
    if(variables.size()>0 && variables.count(key)>0){//search dictionary
        variables[key]=val; //overwrite exsisting value
    }else if(parent !=nullptr){
        parent->setVariable(key,val); //search recursively
    }else{
        std::cerr << "Create New Variable" << key << std::endl;
        variables[key]=val;

    }
}



std::shared_ptr<Environment> Environment::createNewChild(std::string newname){
        auto child = std::make_shared<Environment>(newname,shared_from_this());
        children.push_back(child);
        return children.back();
    };
};