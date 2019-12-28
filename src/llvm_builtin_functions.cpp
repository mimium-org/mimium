#include "llvm_builtin_functions.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace mimium {
LLVMBuiltin::LLVMBuiltin() {}

LLVMBuiltin::~LLVMBuiltin() {}

using builtintype = llvm::Value* (*)(std::vector<llvm::Value*>&, std::string,
                                     std::shared_ptr<LLVMGenerator>);

const std::map<std::string, builtintype> LLVMBuiltin::builtin_fntable = {
    {"print", &LLVMBuiltin::print}, {"sin", &sin}};

const bool LLVMBuiltin::isBuiltin(std::string str) {
  return (builtin_fntable.count(str) > 0);
}

llvm::Value* LLVMBuiltin::print(std::vector<llvm::Value*>& args,
                                std::string name,
                                std::shared_ptr<LLVMGenerator> generator) {
  llvm::Value* format = nullptr;
  auto* fun = generator->module->getFunction("printf");
  if (fun == nullptr) {
    llvm::Type* iptr_type = llvm::PointerType::get(
        llvm::Type::getInt8Ty(generator->module->getContext()), 0);
    llvm::Type* rettype =
        llvm::Type::getInt32Ty(generator->module->getContext());
    llvm::Type* dtype =
        llvm::Type::getDoubleTy(generator->module->getContext());
    std::vector<llvm::Type*> atype = {iptr_type, dtype};
    llvm::FunctionType* ftype = llvm::FunctionType::get(rettype, atype, false);
    // ftype->dump();
    auto* newfun = llvm::cast<llvm::Function>(
        generator->module->getOrInsertFunction("printf", ftype).getCallee());
    newfun->setCallingConv(llvm::CallingConv::C);
    format = generator->builder->CreateGlobalStringPtr("%f\n", "printfformat");
    fun = newfun;
  }
  auto* formatptr = generator->module->getGlobalVariable("printfformat", true);
  auto* castedformat = generator->builder->CreatePointerCast(
      formatptr, fun->arg_begin()->getType(), "ptrto_format");
  args.insert(args.begin(), castedformat);
  llvm::ArrayRef<llvm::Value*> newargs(args);
  auto* res = generator->builder->CreateCall(fun, newargs, name);
  return res;
}
llvm::Value* LLVMBuiltin::sin(std::vector<llvm::Value*>& args,
                                std::string name,
                                std::shared_ptr<LLVMGenerator> generator) {
  llvm::Value* format = nullptr;
  auto* ftype = llvm::FunctionType::get(generator->builder->getDoubleTy(),generator->builder->getDoubleTy(),false);
  auto* fun = llvm::cast<llvm::Function>(
        generator->module->getOrInsertFunction("sin", ftype).getCallee());
  llvm::ArrayRef<llvm::Value*> newargs(args);
  auto* res = generator->builder->CreateCall(fun, newargs, name);
  return res;
}

}  // namespace mimium