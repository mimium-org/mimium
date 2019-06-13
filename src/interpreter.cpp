
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
    auto body  = assign->getBody();
    if(body){
    currentenv->getVariables()[varname] =  body; //share
        return true;
    }else{
        throw  std::runtime_error("expression not resolved");
    }
    }catch(std::exception e){
        std::cerr<<e.what()<<std::endl;
        return false;
    }
}

bool Interpreter::interpretFdef(AST_Ptr line){
    return false;
}

mValue Interpreter::interpretExpr(AST_Ptr expr){
    switch(expr->getid()){
        case SYMBOL:
            return interpretVariable(expr);
        break;
        case NUMBER:
            return interpretNumber(expr);
        break;
        case OP:
            return interpretBinaryExpr(expr);
        default:
            std::cerr << "invalid expression" <<std::endl;
            return 0.0;
    }
}

mValue Interpreter::interpretBinaryExpr(AST_Ptr expr){
    auto var  = std::dynamic_pointer_cast<OpAST>(expr);
    mValue lhs = interpretExpr(var->lhs);
    mValue rhs = interpretExpr(var->rhs);
    switch (var->getOpId()){
        case ADD:
            return lhs + rhs;
            break;
        case SUB:
            return lhs - rhs;
            break;
        case MUL:
            return lhs * rhs;
            break;
        case DIV:
            return lhs / rhs;                                     
            break;
        default: 
            std::cerr << "invalid operator" <<std::endl;
            return 0.0;        
    }
}

mValue Interpreter::interpretVariable(AST_Ptr symbol){
    try{
    auto var  = std::dynamic_pointer_cast<SymbolAST>(symbol);
        return interpretExpr( currentenv->findVariable(var->getVal()) );
    }catch(std::exception e){
        std::cerr<< "Variable not defined" <<std::endl;
        return false;
    }
}

mValue Interpreter::interpretNumber(AST_Ptr num){
    try{
    auto var  = std::dynamic_pointer_cast<NumberAST>(num);
        return  var->getVal();
    }catch(std::exception e){
        std::cerr<< e.what()<<std::endl;
        return 0.0;
    }
}

}//mimium ns