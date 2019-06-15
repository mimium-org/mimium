
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
        tmpres=  interpretAssign(line); // currently function definition is converted into lambda function definition
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



mValue Interpreter::interpretExpr(AST_Ptr expr){
    switch(expr->getid()){
        case SYMBOL:
            return interpretExpr( interpretVariable(expr) );
        break;
        case NUMBER:
            return interpretNumber(expr);
        break;
        case OP:
            return interpretBinaryExpr(expr);
        break;
        case LAMBDA:
            return interpretLambda(expr);
        break;
        case FCALL:
            return interpretFcall(expr);
        break;
        default:
            std::cerr << "invalid expression" <<std::endl;
            return 0.0;
    }
}
struct binary_visitor{
    double operator()(double lhs){return lhs;}
    double operator()(AST_Ptr lhs){
        std::cerr << "invarid binary operation!" << std::endl;
        return 0.0;
    }
};
mValue Interpreter::interpretBinaryExpr(AST_Ptr expr){
    auto var  = std::dynamic_pointer_cast<OpAST>(expr);
    mValue lhs = interpretExpr(var->lhs);
    double lv = std::visit(binary_visitor{},lhs);
    mValue rhs = interpretExpr(var->rhs);
    double rv = std::visit(binary_visitor{},rhs);

    switch (var->getOpId()){
        case ADD:
            return lv + rv;
            break;
        case SUB:
            return lv - rv;
            break;
        case MUL:
            return lv * rv;
            break;
        case DIV:
            return lv / rv;                             
            break;
        default: 
            std::cerr << "invalid operator" <<std::endl;
            return 0.0;        
    }
}

AST_Ptr Interpreter::interpretVariable(AST_Ptr symbol){
    try{
    auto var  = std::dynamic_pointer_cast<SymbolAST>(symbol);
        return currentenv->findVariable(var->getVal());
    }catch(std::exception e){
        std::cerr<< "Variable not defined" <<std::endl;
        return nullptr;
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
mValue Interpreter::interpretLambda(AST_Ptr expr){
    try{
        auto lambda  = std::dynamic_pointer_cast<LambdaAST>(expr);

        return  res;
    }catch(std::exception e){
        std::cerr<< e.what()<<std::endl;
        return 0.0;
    }
}
mValue Interpreter::interpretFcall(AST_Ptr expr){
    try{
    auto fcall  = std::dynamic_pointer_cast<FcallAST>(expr);
    auto name  =  fcall->getFname();
    auto args = fcall->getArgs()->getArgs();
    auto lambda = std::dynamic_pointer_cast<LambdaAST>(interpretVariable(name));
    auto lambdaargs = std::dynamic_pointer_cast<ArgumentsAST>(lambda->getArgs())->getArgs();

    auto body  = lambda->getBody();
    currentenv = currentenv->createNewChild(name->getVal()); //switch
    int argscond = lambdaargs.size() - args.size();
    if(argscond<0){
        throw std::runtime_error("too many arguments");
    }else {
        int count = 0;
        for (auto& larg:lambdaargs ){
            std::string key = std::dynamic_pointer_cast<SymbolAST>(larg)->getVal();
            currentenv->getVariables()[key] = args[count]; //currently only Number,we need to define LHS
            count++;
        }
        if(argscond==0){
            auto tmp = lambda->getBody();
            auto res = interpretExpr(tmp);
            currentenv = currentenv->getParent();//switch back env
            return res;
        }else{
            throw std::runtime_error("too few arguments"); //ideally we want to return new function like closure
        }
    }
    }catch(std::exception e){
        std::cerr<< e.what()<<std::endl;
        return 0.0;
    }
}
struct getdouble_visitor{
    double operator()(double v){return v;};
    double operator()(AST_Ptr v){
        std::cerr<< "invalid value" <<std::endl;
        return 0;
        };
};
double Interpreter::get_as_double(mValue v){
    return std::visit(getdouble_visitor{},v);
}


}//mimium ns