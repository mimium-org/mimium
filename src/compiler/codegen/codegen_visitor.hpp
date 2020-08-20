/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "compiler/codegen/llvmgenerator.hpp"

namespace mimium {
class LLVMGenerator;
struct CodeGenVisitor : public std::enable_shared_from_this<CodeGenVisitor> {
  friend LLVMGenerator;
  CodeGenVisitor(LLVMGenerator& g);
  void operator()(NumberInst& i);
  void operator()(StringInst& i);
  void operator()(AllocaInst& i);
  void operator()(RefInst& i);
  void operator()(AssignInst& i);
  // void operator()(TimeInst& i);
  void operator()(OpInst& i);
  void createBinOp(OpInst& i);
  void createUniOp(OpInst& i);
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
  std::string context_hasself;
  llvm::Value* getDirFun(FcallInst& i);
  llvm::Value* getClsFun(FcallInst& i);
  llvm::Value* getExtFun(FcallInst& i);

  llvm::FunctionType* createFunctionType(FunInst& i,bool hascapture,bool hasmemobj);
  llvm::Function* createFunction(llvm::FunctionType* type, FunInst& i);
  void addArgstoMap(llvm::Function* f, FunInst& i,bool hascapture,bool hasmemobj);

  void setFvsToMap(FunInst& i, llvm::Value* clsarg);
  void setMemObjsToMap(FunInst& i, llvm::Value* memarg);

  llvm::Value* createAllocation(bool isglobal, llvm::Type* type,
                                llvm::Value* ArraySize,
                                const llvm::Twine& name);
  bool createStoreOw(std::string varname, llvm::Value* val_to_store);
  void createAddTaskFn(FcallInst& i, bool isclosure, bool isglobal);

  const static std::unordered_map<newast::OpId, std::string> opid_to_ffi;
};
}  // namespace mimium