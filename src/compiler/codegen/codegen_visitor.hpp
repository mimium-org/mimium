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
  void operator()(mir::NumberInst& i);
  void operator()(mir::StringInst& i);
  void operator()(mir::AllocaInst& i);
  void operator()(mir::RefInst& i);
  void operator()(mir::AssignInst& i);
  void operator()(mir::OpInst& i);
  void createBinOp(mir::OpInst& i);
  void createUniOp(mir::OpInst& i);
  void operator()(mir::FunInst& i);
  void operator()(mir::FcallInst& i);
  void operator()(mir::MakeClosureInst& i);
  void operator()(mir::ArrayInst& i);
  void operator()(mir::ArrayAccessInst& i);
  void operator()(mir::IfInst& i);
  void operator()(mir::ReturnInst& i);

 private:
  LLVMGenerator& G;
  bool isglobal;
  bool context_hasself;
  llvm::Value* getDirFun(mir::FcallInst& i);
  llvm::Value* getClsFun(mir::FcallInst& i);
  llvm::Value* getExtFun(mir::FcallInst& i);

  llvm::FunctionType* createFunctionType(mir::FunInst& i, bool hascapture, bool hasmemobj);
  llvm::Function* createFunction(llvm::FunctionType* type, mir::FunInst& i);
  void addArgstoMap(llvm::Function* f, mir::FunInst& i, bool hascapture, bool hasmemobj);

  void setFvsToMap(mir::FunInst& i, llvm::Value* clsarg);
  void setMemObjsToMap(mir::FunInst& i, llvm::Value* memarg);
  void setMemObj(llvm::Value* memarg, std::string const& name, int index);
  llvm::Value* createAllocation(bool isglobal, llvm::Type* type, llvm::Value* ArraySize,
                                const llvm::Twine& name);
  bool createStoreOw(std::string varname, llvm::Value* val_to_store);
  void createAddTaskFn(mir::FcallInst& i, bool isclosure, bool isglobal);
  void createIfBody(mir::blockptr& block, llvm::Value* ret_ptr);
  const static std::unordered_map<ast::OpId, std::string> opid_to_ffi;
};
}  // namespace mimium