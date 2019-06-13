#include "llvmgenerator.hpp"
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

bool LLVMGenerator::generateCode(std::unique_ptr<ListAST> listast,std::string name){
    bool res = false;
    for (auto& statements : listast->getlist()){
        switch(statements.id){
            case FDEF:
                res  = generateFdef(std::move(statements));
                break;
            case ASSIGN:
                res =  generateAssign(std::move(statements));
                break;
            default:
                std::cerr<<"invalid AST id: " << statements.id << std::endl;
                break; 
        }
    }
    return res;
}

llvm::Value* LLVMGenerator::generateNumber(AST_Ptr num){
    if(num->getid()!=NUMBER){
        std::cerr<<"invalid AST id,expected NUMBER but: " << num->getid() << std::endl;
        return nullptr;
    }else{
        return llvm::ConstantFP::get(ctx, APFloat((float)num->val));
    }
}

llvm::Value* LLVMGenerator::generateOpExpr(AST_Ptr expr){
    llvm::Value* L
    llvm::Value* R 
    switch(expr->getid()){
        case OP:
            L = generateOpExpr(expr->lhs);
            R = generateOpExpr(expr->rhs);
            if (!L || !R)
                return nullptr;
            switch(expr->op_id){
                case ADD:
                    return Builder.CreateFAdd(L, R, "addtmp");
                case SUB:
                    return Builder.CreateFSub(L, R, "subtmp");
                case MUL:
                    return Builder.CreateFMul(L, R, "multmp");
                case DIV:
                    return Builder.CreateFDiv(L, R, "divtmp");
                default:
                std::cerr<<"invalid Binary Operator: " << num->getid() << std::endl;
                    return nullptr;
            }
            break;
        case NUMBER:
            return generateNumber(expr);//fall back to number
            break;
        default:
        std::cerr<<"invalid AST id,expected OP but: " << num->getid() << std::endl;
        return nullptr;
    }
}
