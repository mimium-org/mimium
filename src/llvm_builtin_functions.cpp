#include "llvm_builtin_functions.hpp"

namespace mimium {
LLVMBuiltin::LLVMBuiltin(){

}
using builtintype = llvm::Value* (*)(std::vector<llvm::Value*>&,std::string,std::shared_ptr<LLVMGenerator>);

const std::map<std::string,builtintype> LLVMBuiltin::builtin_fntable = {
    {"print", &LLVMBuiltin::print}
    };

const bool LLVMBuiltin::isBuiltin(std::string str) {
  return (builtin_fntable.count(str) > 0);
}

llvm::Value* LLVMBuiltin::print(std::vector<llvm::Value*>& args, std::string name,std::shared_ptr<LLVMGenerator> generator) {
  auto* fun = generator->module->getFunction("printf");
  if (!fun) {
    llvm::Type* iptr_type = llvm::PointerType::get(
        llvm::Type::getInt8Ty(generator->module->getContext()), 0);
    llvm::Type* rettype = llvm::Type::getInt32PtrTy(generator->module->getContext());
    llvm::Type* dtype = llvm::Type::getFloatTy(generator->module->getContext());
    std::vector<llvm::Type*> atype;
    atype.push_back(iptr_type);
    atype.push_back(dtype);
    llvm::FunctionType* ftype =
      llvm::FunctionType::get(rettype,atype ,true);
    generator->setBB(generator->mainentry);
    auto* fun = llvm::Function::Create(ftype ,llvm::GlobalValue::ExternalLinkage,
                                 "printf", generator->module.get());
    fun->setCallingConv(llvm::CallingConv::C);
    llvm::Value* format =
        generator->builder->CreateGlobalStringPtr("%d\n", "printfformat");
  }
  auto* _format =
      generator->module->getGlobalVariable("printfformat");
  args.insert(args.begin(), _format);
    generator->setBB(generator->currentblock);
  return generator->builder->CreateCall(fun, args, name);
}
}  // namespace mimium