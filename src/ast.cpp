
#include "ast.hpp"

std::string NumberAST::to_string(){
        std::stringstream stream;
        stream <<  std::to_string(val);
        if(istimeset()){
            stream << "@" << std::to_string(get_time());
        }
        return stream.str();
    
}

llvm::Value* NumberAST::codegen(){
    return llvm::ConstantFP::get(TheContext, llvm::APFloat((float)val));
}

std::string OpAST::to_string(){
        std::stringstream stream;
        stream << "("<< op <<" " <<lhs->to_string() << " " << rhs->to_string() <<")";
        if(istimeset()){
            stream << "@" << std::to_string(get_time());
        }
        return stream.str();
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

std::string ListAST::to_string(){
        std::stringstream stream;
        stream << "(";
        for(auto &elem :asts){
            stream << elem->to_string() << " ";
        }
        stream << ")";
        if(istimeset()){
            stream << "@" << std::to_string(get_time());
        }
        return stream.str();
    }

llvm::Value* ListAST::codegen(){
    return LogErrorV("not implemented yet");
}
