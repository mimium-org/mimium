
#include "interpreter.hpp"
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

std::string Closure::to_string(){
    std::stringstream ss;
    ss << "Closure:<";
    fun->to_string(ss);
    ss << " , " << env->getName(); 
    return ss.str();
}

mValue Interpreter::loadAst(AST_Ptr _ast){
    topast  = _ast;
    return interpretTopAst();
}

mValue Interpreter::interpretListAst(AST_Ptr ast){
    mValue res;
    switch (ast->getid()){
        case LIST:
        for(auto& line: std::dynamic_pointer_cast<ListAST>(ast)->getlist()){
            res = interpretStatementsAst(line);
            if(line->getid() == RETURN) break;
        }
        break;
        default:
            res = interpretStatementsAst(ast);
        break;
    }
    return res;
}

mValue Interpreter::interpretStatementsAst(AST_Ptr line){
    mValue tmpres=false;

    switch (line->getid())
    {
    case ASSIGN:
        tmpres =  interpretAssign(line);
        break;
    // fdef is directly converted into assign lambda
    // case FDEF: 
    //     tmpres=  interpretFdef(line); 
    //     break;
    case RETURN:
        tmpres = interpretReturn(line);
        goto end;
    default: 
        tmpres= interpretExpr(line);
        break;
    }
    end:
    return tmpres;
}

mValue Interpreter::interpretReturn(AST_Ptr line){
   try{
   auto ret =  std::dynamic_pointer_cast<ReturnAST>(line);
   return interpretExpr(ret->getExpr());
   }catch(std::exception e){
        std::cerr<<e.what()<<std::endl;
        return false;
    }
}

mValue Interpreter::interpretAssign(AST_Ptr line){
    try{
    auto assign  = std::dynamic_pointer_cast<AssignAST>(line);
    std::string varname = assign->getName()->getVal();
    if(currentenv->isVariableSet(varname)){
        std::cout<<"Variable "<< varname << " already exists. Overwritten"<<std::endl;
    }
    auto body  = assign->getBody();
    if(body){
        currentenv->setVariable(varname, interpretExpr(body)); //share
      return line; //for print
    }else{
        throw  std::runtime_error("expression not resolved");
    }
    }catch(std::exception e){
        std::cerr<<e.what()<<std::endl;
        return false;
    }
}
// mValue Interpreter::interpretFdef(AST_Ptr line){
//     try{
//         auto fdef = std::dynamic_pointer_cast<FdefAST>(line);
//         std::string fname = fdef->getFname()->getVal();
//         currentenv = currentenv->createNewChild(fname); //switch
//         mValue interres = interpretStatementsAst(fdef->getFbody());

//     }catch(std::exception e){
//         std::cerr<<e.what()<<std::endl;
//         return false;
//     }
// }


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
        mValue val = (lhs);
        std::cerr << "invarid binary operation!" << std::endl;
        return 0.0;
    }
    double operator()(mClosure_ptr lhs){
        mValue val = (lhs);
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

mValue Interpreter::interpretVariable(AST_Ptr symbol){
    try{
    auto var  = std::dynamic_pointer_cast<SymbolAST>(symbol);
        return currentenv->findVariable(var->getVal());
    }catch(std::exception e){
        std::cerr<< "Variable not defined" <<std::endl;
        return 0;
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
        auto lambda = std::dynamic_pointer_cast<LambdaAST>(expr);
        auto closure = std::make_shared<Closure>(currentenv,lambda);
        // std::cout << "Closure created" << currentenv->getName();
        
        // lambda->to_string(std::cout);
        return std::move(closure);
    }catch(std::exception e){
        std::cerr<< e.what()<<std::endl;
        return 0;
    }
}
struct fcall_visitor{
    mClosure_ptr operator()(double v){
        throw std::runtime_error("reffered variable is not a function");
        return nullptr;
        };
    mClosure_ptr operator()(AST_Ptr v){
        throw std::runtime_error("reffered variable is not a closure");
        return nullptr;
    };
    mClosure_ptr operator()(std::shared_ptr<Closure> v){
        return v;
        };
};
mValue Interpreter::interpretFcall(AST_Ptr expr){
    try{
    auto fcall  = std::dynamic_pointer_cast<FcallAST>(expr);
    auto name  =  std::dynamic_pointer_cast<SymbolAST>(fcall->getFname())->getVal();
    auto args = fcall->getArgs()->getArgs();
    mValue var = findVariable(name);
    mClosure_ptr closure  =std::visit(fcall_visitor{},var);
    auto lambda = closure->fun;
    std::shared_ptr<Environment> tmpenv = currentenv; 
    currentenv = closure->env; //switch to closure context
    // std::cout << "switched to closure context: " << currentenv->getName()<<std::endl;
    // std::cout << "localvar"<< to_string(currentenv->findVariable("localvar")) <<std::endl;

    auto lambdaargs = std::dynamic_pointer_cast<ArgumentsAST>(lambda->getArgs())->getArgs();

    auto body  = lambda->getBody();
    currentenv = currentenv->createNewChild(name); //create arguments
    int argscond = lambdaargs.size() - args.size();
    if(argscond<0){
        throw std::runtime_error("too many arguments");
    }else {
        int count = 0;
        for (auto& larg:lambdaargs ){
            std::string key = std::dynamic_pointer_cast<SymbolAST>(larg)->getVal();
            currentenv->getVariables()[key] = std::dynamic_pointer_cast<NumberAST>(args[count])->getVal(); //currently only Number,we need to define LHS
            count++;
        }
        if(argscond==0){
            auto tmp = lambda->getBody();
            auto res = interpretListAst(tmp);
            currentenv = tmpenv;//switch back env
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
    double operator()(mClosure_ptr v){
        std::cerr<< "invalid value" <<std::endl;
        return 0;
        };
};
double Interpreter::get_as_double(mValue v){
    return std::visit(getdouble_visitor{},v);
};
struct tostring_visitor{
    std::string operator()(double v){return std::to_string(v);};
    std::string operator()(AST_Ptr v){
        std::stringstream ss;
        v->to_string(ss);
        return ss.str();
        };
    std::string operator()(mClosure_ptr v){
        return v->to_string();
        };
};
std::string Interpreter::to_string(mValue v){
    return std::visit(tostring_visitor{},v);
};

}//mimium ns