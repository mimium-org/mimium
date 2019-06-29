
#include "interpreter.hpp"

namespace mimium{

void Interpreter::start(){
    sch->start();
}

void Interpreter::stop(){
    sch->stop();
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
    case IF:
        tmpres = interpretIf(line);
        break;
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
        // std::cout<<"Variable "<< varname << " already exists. Overwritten"<<std::endl;
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
        case ARRAY:
            return interpretArray(expr);
        break;
        case TIME:
            return interpretTime(expr);
        break;
        default:
            std::cerr << "invalid expression" <<std::endl;
            return 0.0;
    }
}
overloaded binary_visitor{
    [](double lhs){return lhs;},
    [](auto lhs){
        mValue val = (lhs);
        std::cerr << "invarid binary operation!" << std::endl;
        return 0.0;
    }
};
mValue Interpreter::interpretBinaryExpr(AST_Ptr expr){
    auto var  = std::dynamic_pointer_cast<OpAST>(expr);
    mValue lhs = interpretExpr(var->lhs);
    double lv = std::visit(binary_visitor,lhs);
    mValue rhs = interpretExpr(var->rhs);
    double rv = std::visit(binary_visitor,rhs);

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
overloaded fcall_visitor{
    [](auto v)->mClosure_ptr{
        throw std::runtime_error("reffered variable is not a function");
        return nullptr;
        },
    [](std::shared_ptr<Closure> v)->mClosure_ptr{
        return v;
        }
};
mValue Interpreter::interpretFcall(AST_Ptr expr){
    try{
    auto fcall  = std::dynamic_pointer_cast<FcallAST>(expr);

    auto name  =  std::dynamic_pointer_cast<SymbolAST>(fcall->getFname())->getVal();
    auto args = fcall->getArgs()->getElements();
    if(mimium::builtin::isBuiltin(name)){
        auto fn = mimium::builtin::builtin_fntable.at(name);
        fn(interpretExpr(args[0])); // currently implemented only for print()
        return 0.0;
    }else{
        mValue var = findVariable(name);
        mClosure_ptr closure  =std::visit(fcall_visitor,var);
        auto lambda = closure->fun;
        std::shared_ptr<Environment> tmpenv = currentenv; 
        currentenv = closure->env; //switch to closure context
        auto lambdaargs = std::dynamic_pointer_cast<ArgumentsAST>(lambda->getArgs())->getElements();

        auto body  = lambda->getBody();
        currentenv = currentenv->createNewChild(name); //create arguments
        int argscond = lambdaargs.size() - args.size();
        if(argscond<0){
            throw std::runtime_error("too many arguments");
        }else {
            int count = 0;
            for (auto& larg:lambdaargs ){
                std::string key = std::dynamic_pointer_cast<SymbolAST>(larg)->getVal();
                currentenv->getVariables()[key] = interpretExpr(args[count]);//currently only Number,we need to define LHS
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
    }
    }catch(std::exception e){
        std::cerr<< e.what()<<std::endl;
        return 0.0;
    }
}
mValue Interpreter::interpretArray(AST_Ptr array){
    try{
    auto arr = std::dynamic_pointer_cast<ArrayAST>(array);
    std::vector<double> v;
    for (auto &elem :arr->getElements()){
        double res = get_as_double(interpretExpr(elem));
        v.push_back(res);
    }
    return std::move(v);
    }catch(std::exception e){
        std::cerr<< e.what()<<std::endl;
        return 0.0;
    } 
}
mValue Interpreter::interpretIf(AST_Ptr expr){
    try{
    auto ifexpr = std::dynamic_pointer_cast<IfAST>(expr);
    mValue cond = interpretExpr(ifexpr->getCond());
    auto cond_d = get_as_double(cond);
    if(cond_d!=0){
                return interpretListAst(ifexpr->getThen());
            }else{
                return interpretListAst(ifexpr->getElse());
    }
    }catch(std::exception e){
        std::cerr<< e.what()<<std::endl;
        return 0.0;
    } 
}


mValue Interpreter::interpretTime(AST_Ptr expr){
    try{
    auto timeexpr = std::dynamic_pointer_cast<TimeAST>(expr);
    mValue time = interpretExpr(timeexpr->getTime());
    std::visit(overloaded {
        [&](double t){sch->addTask(t, timeexpr->getExpr());},
        [](auto t){throw std::runtime_error("you cannot append value pther than double");}
    },time);
    return 0;
    }catch(std::exception e){
        std::cerr<< e.what()<<std::endl;
        return 0.0;
    }
}

double Interpreter::get_as_double(mValue v){
    return std::visit(overloaded{
        [](double v)->double{return v;},
        [](auto v)->double{
        std::cerr<< "invalid value" <<std::endl;
        return 0;
    }
    },v);
};

std::string Interpreter::to_string(mValue v){
    return std::visit(overloaded{
        [](double v){return std::to_string(v);},
        [](AST_Ptr v){
            std::stringstream ss;
            v->to_string(ss);
            return ss.str();
        },
        [](mClosure_ptr v){
        return v->to_string();
        },
        [](std::vector<double> vec){
        std::stringstream ss;
        ss << "[";
        int count = vec.size();
        for (auto &elem: vec ){
            ss<<elem;
            count--;
            if (count>0) ss<<",";
        }
        ss<<"]";
        return ss.str();
        }
    },v);
};

}//mimium ns