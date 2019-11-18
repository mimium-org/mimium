#include "llvmgenerator.hpp"
namespace mimium{
LLVMGenerator::LLVMGenerator(std::string _filename){
    builder = std::make_unique<llvm::IRBuilder<>>(ctx);
    module =std::make_shared<llvm::Module>(_filename,ctx);;
}
// LLVMGenerator::LLVMGenerator(llvm::LLVMContext& _ctx,std::string _filename){
//     // ctx.reset();
//     // ctx = std::move(&_ctx);
// }

LLVMGenerator::~LLVMGenerator(){}


std::shared_ptr<llvm::Module> LLVMGenerator::getModule(){
    if(module){
        return module;
    }else{
        return std::make_shared<llvm::Module>("null",ctx);
    }
}

void LLVMGenerator::preprocess(){
    auto* fntype =llvm::FunctionType::get(llvm::Type::getInt64Ty(ctx), false);
    auto* mainfun = llvm::Function::Create(fntype,llvm::Function::ExternalLinkage, "main", module.get());
    builder->SetInsertPoint(llvm::BasicBlock::Create(ctx, "entry", mainfun));

}

void LLVMGenerator::generateCode(std::shared_ptr<MIRblock> mir){
    preprocess();
    for(auto& inst : mir->instructions){
        std::visit(overloaded{
            [](auto i){ },
            [&,this](std::shared_ptr<NumberInst> i){
                auto ptr = builder->CreateAlloca(llvm::Type::getDoubleTy(ctx), nullptr, i->lv_name);
                namemap.emplace(i->lv_name,ptr);
                auto finst = llvm::ConstantFP::get(this->ctx,llvm::APFloat(i->val));
                builder->CreateStore(finst,ptr);
            },
            [&,this](std::shared_ptr<OpInst> i){
                llvm::Value* ptr;
                switch (i->getOPid())
                {
                case ADD:
                    ptr = builder->CreateFAdd(namemap[i->lhs],namemap[i->rhs],i->lv_name);
                    break;
                case SUB:
                    ptr = builder->CreateFSub(namemap[i->lhs],namemap[i->rhs],i->lv_name);
                    break;
                case MUL:
                    ptr = builder->CreateFMul(namemap[i->lhs],namemap[i->rhs],i->lv_name);
                    break;
                case DIV:
                    ptr = builder->CreateFDiv(namemap[i->lhs],namemap[i->rhs],i->lv_name);
                    break;                                        
                default:
                    break;
                }
                namemap.emplace(i->lv_name,ptr);                    
            }
        },inst);
    }

      builder->CreateRet(builder->getInt64(0));

}

void LLVMGenerator::outputToStream(llvm::raw_ostream& stream){
    module->print(stream,nullptr,false,true);
}

}//namespace mimium;