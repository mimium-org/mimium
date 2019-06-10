
#include "ast.hpp"


std::ostream& NumberAST::to_string(std::ostream& ss){
        ss <<  std::to_string(val);
        if(istimeset()){
            ss << "@" << std::to_string(get_time());
        }
        return ss;
    
}

llvm::Value* NumberAST::codegen(){
    return llvm::ConstantFP::get(TheContext, llvm::APFloat((float)val));
}

std::ostream& SymbolAST::to_string(std::ostream& ss){
    ss <<  val;
    if(istimeset()){
        ss << "@" << std::to_string(get_time());
    }
    return ss;
}
llvm::Value* SymbolAST::codegen(){
    return LogErrorV("not implemented yet");
}


std::ostream& OpAST::to_string(std::ostream& ss){
        ss << "("<< op <<" ";
        lhs->to_string(ss);
        ss<< " ";
        rhs->to_string(ss);
        ss<<")";
        if(istimeset()){
            ss<< "@" << std::to_string(get_time());
        }
        return ss;
    }
auto OpAST::codegen_pre(){
    auto res =  std::make_pair(lhs->codegen(),rhs->codegen());
    return res;
}
llvm::Value* AddAST::codegen(){
    auto lr  = codegen_pre();
    Builder.CreateFAdd(lr.first, lr.second, "addtmp");
    return Builder.CreateUIToFP(lr.first, llvm::Type::getDoubleTy(TheContext), "booltmp");
}
llvm::Value* SubAST::codegen(){
    auto lr  = codegen_pre();
    Builder.CreateFSub(lr.first, lr.second, "subtmp");
    return Builder.CreateUIToFP(lr.first, llvm::Type::getDoubleTy(TheContext), "booltmp");
}

llvm::Value* MulAST::codegen(){
    auto lr  = codegen_pre();
    Builder.CreateFMul(lr.first, lr.second, "multmp");
    return Builder.CreateUIToFP(lr.first, llvm::Type::getDoubleTy(TheContext), "booltmp");
}
llvm::Value* DivAST::codegen(){
    auto lr  = codegen_pre();
    Builder.CreateFDiv(lr.first, lr.second, "divtmp");
    return Builder.CreateUIToFP(lr.first, llvm::Type::getDoubleTy(TheContext), "booltmp");
}

std::ostream& ListAST::to_string(std::ostream& ss){
        ss << "(";
        for(auto &elem :asts){
            elem->to_string(ss);
            ss<< " ";
        }
        ss << ")";
        if(istimeset()){
            ss << "@" << std::to_string(get_time());
        }
        return ss;
    }

llvm::Value* ListAST::codegen(){
    return LogErrorV("not implemented yet");
}
std::ostream& ArgumentsAST::to_string(std::ostream& ss){
        ss << "(";
        for(auto &elem :args){
            elem->to_string(ss);
            ss<< " ";
        }
        ss << ")";
        if(istimeset()){
            ss << "@" << std::to_string(get_time());
        }
        return ss;
    }

llvm::Value* ArgumentsAST::codegen(){
    return LogErrorV("not implemented yet");
}

std::ostream& LambdaAST::to_string(std::ostream& ss){
        ss << "(lambda (";
        args->to_string(ss);
        ss <<")";
        body->to_string(ss) ;
        ss << ")";
        if(istimeset()){
            ss << "@" << std::to_string(get_time());
        }
        return ss;
    }

llvm::Value* LambdaAST::codegen(){
    return LogErrorV("not implemented yet");
}

std::ostream& AssignAST::to_string(std::ostream& ss){
        ss << "("<< "assign" <<" ";
        symbol->to_string(ss);
        ss << " ";
        expr->to_string(ss);
        ss <<")";
        if(istimeset()){
            ss << "@" << std::to_string(get_time());
        }
        return ss;
}
llvm::Value* AssignAST::codegen(){
    return LogErrorV("not implemented yet");
}
