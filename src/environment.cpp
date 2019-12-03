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

std::pair<bool,bool> Environment::isFreeVariable(std::string key){
    //return value: [isvarset , isfv]
    if(variables.size()>0 && variables.count(key)>0){//search dictionary
        return std::pair(true,false); //return false if var is local variable
    }else if(parent !=nullptr){
        auto [isvarset,isisfv] = parent->isFreeVariable(key);
        return std::pair(isvarset,true); //search recursively
    }else{
        throw std::runtime_error("Variable " + key  +" not found");    
        return std::pair(false,false);
    }
}
mValue Environment::findVariable(std::string key){
    if(variables.size()>0 && variables.count(key)>0){//search dictionary
        return variables.at(key);
    }else if(parent !=nullptr){
        return parent->findVariable(key); //search recursively
    }else{

        throw std::runtime_error("Variable " + key  +" not found");    
        return 0.0;
    }
}


void Environment::setVariable(std::string key,mValue val){
    mValue newval=0.0;
    if(auto pval = std::get_if<std::string>(&val)){ 
        newval = findVariable(*pval);
    }else{
        newval = val;
    }
    if(variables.size()>0 && variables.count(key)>0){//search dictionary
        // Logger::debug_log("Variable " + key + " already exists as " + variables[key] + ". Overwritten to " + Runtime::to_string(newval),
        //               Logger::INFO);
        Logger::debug_log("overwritten",Logger::DEBUG);
        variables[key] = newval; //overwrite exsisting value

    }else if(parent !=nullptr){
        parent->setVariable(key,newval); //search recursively
    }else{
        Logger::debug_log( "Create New Variable " + key  ,Logger::DEBUG);
        variables.emplace(key,newval);
    }
}
void Environment::setVariableRaw(std::string key,mValue val){
            Logger::debug_log( "Create New Argument " + key,Logger::DEBUG);
            variables.emplace(key,val);
}



std::shared_ptr<Environment> Environment::createNewChild(std::string newname){
        auto child = std::make_shared<Environment>(newname,shared_from_this());
        children.push_back(child);
        return children.back();
}

void Environment::deleteLastChild(){
    children.pop_back();
};

}//namespace mimium


