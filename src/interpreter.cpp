
#include "interpreter.hpp"
namespace mimium{
AST_Ptr Environment::findVariable(std::string key){
    if(variables.count(key)){//search dictionary
        return variables.at(key);
    }else if(parent !=nullptr){
        return parent->findVariable(key); //search recursively
    }else{
        std::cerr << "Variable" << key << "not found" << std::endl;
        return nullptr;
    }
}

std::shared_ptr<Environment> Environment::createNewChild(std::string newname){
        auto child = std::make_shared<Environment>(newname,shared_from_this());
        children.push_back(child);
        return children.back();
    };

bool Interpreter::loadAst(AST_Ptr _ast){
    ast  = std::move(_ast);
    return interpretTopAst();
}

bool Interpreter::interpretTopAst(){
    for(auto& line: std::dynamic_pointer_cast<ListAST>(ast)->getlist()){
    bool tmpres=false;
    switch (line->getid())
    {
    case ASSIGN:
        tmpres=  interpretAssign(line);
        break;

    case FDEF:
    std::cerr<< "notimplemented" <<std::endl;
        break;
    default: 
        break;
    }
    res = res | tmpres;
    }
    return res;
}

bool Interpreter::interpretAssign(AST_Ptr line){
    try{
    auto assign  = std::dynamic_pointer_cast<AssignAST>(line);
    std::string varname = assign->getName()->getVal();
    if(currentenv->getVariables().count(varname)){
        std::cout<<"Variable "<< varname << " already exists. Overwritten"<<std::endl;
    }
    currentenv->getVariables()[varname] = assign->getBody();
    return true;
    }catch(std::exception e){
        std::cerr<<e.what()<<std::endl;
        return false;
    }
}

bool Interpreter::interpretFdef(AST_Ptr line){
    return false;
}

}//mimium ns