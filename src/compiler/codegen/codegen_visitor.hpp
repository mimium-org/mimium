/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "compiler/codegen/llvmgenerator.hpp"

namespace mimium {
namespace minst = mir::instruction;
class LLVMGenerator;
struct CodeGenVisitor : public std::enable_shared_from_this<CodeGenVisitor> {
  friend LLVMGenerator;
  CodeGenVisitor(LLVMGenerator& g);
  llvm::Value* operator()(minst::Number& i);
  llvm::Value* operator()(minst::String& i);
  llvm::Value* operator()(minst::Allocate& i);
  llvm::Value* operator()(minst::Ref& i);
  llvm::Value* operator()(minst::Load& i);
  llvm::Value* operator()(minst::Store& i);
  llvm::Value* operator()(minst::Op& i);
  llvm::Value* createBinOp(minst::Op& i);
  llvm::Value* createUniOp(minst::Op& i);
  llvm::Value* operator()(minst::Function& i);
  llvm::Value* operator()(minst::Fcall& i);
  llvm::Value* operator()(minst::MakeClosure& i);
  llvm::Value* operator()(minst::Array& i);
  llvm::Value* operator()(minst::Field& i);
  llvm::Value* operator()(minst::If& i);
  llvm::Value* operator()(minst::Return& i);

 private:
  LLVMGenerator& G;
  bool isglobal;
  bool context_hasself;
  llvm::Value* getDirFun(minst::Fcall& i);
  llvm::Value* getClsFun(minst::Fcall& i);
  llvm::Value* getExtFun(minst::Fcall& i);

  llvm::FunctionType* createFunctionType(minst::Function& i, bool hascapture, bool hasmemobj);
  llvm::Function* createFunction(llvm::FunctionType* type, minst::Function& i);
  void addArgstoMap(llvm::Function* f, minst::Function& i, bool hascapture, bool hasmemobj);

  void setFvsToMap(minst::Function& i, llvm::Value* clsarg);
  void setMemObjsToMap(minst::Function& i, llvm::Value* memarg);
  void setMemObj(llvm::Value* memarg, std::string const& name, int index);
  llvm::Value* createAllocation(bool isglobal, llvm::Type* type, llvm::Value* ArraySize,
                                const llvm::Twine& name);
  bool createStoreOw(std::string varname, llvm::Value* val_to_store);
  void createAddTaskFn(minst::Fcall& i, bool isclosure, bool isglobal);
  void createIfBody(mir::blockptr& block, llvm::Value* ret_ptr);
  const static std::unordered_map<ast::OpId, std::string> opid_to_ffi;
};
}  // namespace mimium