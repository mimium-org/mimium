#pragma once
#include "compiler/codegen/llvmgenerator.hpp"

namespace mimium {
class LLVMGenerator;
struct CodeGenVisitor :public std::enable_shared_from_this<CodeGenVisitor>{
  friend LLVMGenerator;
  CodeGenVisitor(LLVMGenerator& g);
  void operator()(NumberInst& i);
  void operator()(AllocaInst& i);
  void operator()(RefInst& i);
  void operator()(AssignInst& i);
  void operator()(TimeInst& i);
  void operator()(OpInst& i);
  void operator()(FunInst& i);
  void operator()(FcallInst& i);
  void operator()(MakeClosureInst& i);
  void operator()(ArrayInst& i);
  void operator()(ArrayAccessInst& i);
  void operator()(IfInst& i);
  void operator()(ReturnInst& i);

 private:
  LLVMGenerator& G;
  bool isglobal;

  llvm::Value* getDirFun(FcallInst& i);
  llvm::Value* getClsFun(FcallInst& i);
  llvm::Value* getExtFun(FcallInst& i);

  llvm::Function* createFunction(llvm::FunctionType* type, FunInst& i);
  void addArgstoMap(llvm::Function* f, FunInst& i);

  void setFvsToMap(FunInst& i, llvm::Function* f);
  llvm::Value* createAllocation(bool isglobal, llvm::Type* type,
                                llvm::Value* ArraySize,
                                const llvm::Twine& name);
  bool createStoreOw(std::string varname, llvm::Value* val_to_store);
  void createAddTaskFn(FcallInst& i, bool isclosure, bool isglobal);
};
}  // namespace mimium