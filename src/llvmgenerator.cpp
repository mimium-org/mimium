#include "llvmgenerator.hpp"
namespace mimium{
LLVMGenerator::LLVMGenerator(){
    builder = std::make_unique<llvm::IRBuilder<>>(ctx);
    module =nullptr;
}
LLVMGenerator::LLVMGenerator(llvm::LLVMContext& _ctx){
    ctx.reset()
    ctx = std::move(&_ctx);
    LLVMGenerator();
}


std::shared_ptr<llvm::Module> LLVMGenerator::getModule(){
    if(module){
        return module;
    }else{
        std::make_shared<llvm::Module>("null",ctx);
    }
}

bool LLVMGenerator::generateCode(ListAST& listast,std::string name){
    listast.accept(alphavisitor);
    alphavisitor.getResult().accept(knormvisitor);
    knormvisitor.getResult().accept(closurevisitor);
    closurevisitor.getResult().accept(this);
}

void LLVMVisitor::visit(OpAST& ast){
    
};
void LLVMVisitor::visit(ListAST& ast){
    
};
void LLVMVisitor::visit(NumberAST& ast){
    
};
void LLVMVisitor::visit(SymbolAST& ast){
    
};
void LLVMVisitor::visit(AssignAST& ast){
    
};
void LLVMVisitor::visit(ArgumentsAST& ast){
    
};
void LLVMVisitor::visit(ArrayAST& ast){
    
};
void LLVMVisitor::visit(ArrayAccessAST& ast){
    
};
void LLVMVisitor::visit(FcallAST& ast){
    
};
void LLVMVisitor::visit(LambdaAST& ast){
    
};
void LLVMVisitor::visit(IfAST& ast){
    
};
void LLVMVisitor::visit(ReturnAST& ast){
    
};
void LLVMVisitor::visit(ForAST& ast){
    
};
void LLVMVisitor::visit(DeclarationAST& ast){
    
};
void LLVMVisitor::visit(TimeAST& ast){
    
};

}