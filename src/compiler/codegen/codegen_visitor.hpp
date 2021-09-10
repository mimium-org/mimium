/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <queue>
#include "basic/mir.hpp"
#include "compiler/codegen/llvm_header.hpp"
#include "compiler/collect_memoryobjs.hpp"
namespace mimium {
namespace minst = mir::instruction;

class LLVMGenerator;
struct CodeGenVisitor {
  enum class LlvmOp {
    Add,
    Sub,
    Mul,
    Div,
    // TODO:add llvm math Intrinsics
  };

  friend LLVMGenerator;
  CodeGenVisitor(LLVMGenerator& g, const funobjmap* funobj_map);
  llvm::Value* operator()(minst::NoOp& i);
  llvm::Value* operator()(minst::Number& i);
  llvm::Value* operator()(minst::String& i);
  llvm::Value* operator()(minst::Allocate& i);
  llvm::Value* operator()(minst::Ref& i);
  llvm::Value* operator()(minst::Load& i);
  llvm::Value* operator()(minst::Store& i);
  // llvm::Value* operator()(minst::Op& i);
  // llvm::Value* createBinOp(minst::Op& i);
  // llvm::Value* createUniOp(minst::Op& i);
  llvm::Value* operator()(minst::Function& i);
  llvm::Value* operator()(minst::Fcall& i);
  llvm::Value* operator()(minst::MakeClosure& i);
  llvm::Value* operator()(minst::Array& i);
  llvm::Value* operator()(minst::ArrayAccess& i);

  llvm::Value* operator()(minst::Field& i);
  llvm::Value* operator()(minst::If& i);
  llvm::Value* operator()(minst::Return& i);
  llvm::Value* visit(mir::valueptr val);

 private:
  const static std::unordered_map<std::string_view, LlvmOp> fname_to_op;
  std::optional<LlvmOp> getPrimitiveOp(mir::valueptr fname);
  llvm::Value* generatePrimitiveBinOp(LlvmOp op, const std::string& name, mir::valueptr lhs,
                                      mir::valueptr rhs);
  llvm::Value* generatePrimitiveUniOp(LlvmOp op, const std::string& name, mir::valueptr rhs);

  void registerLlvmVal(mir::valueptr mirval, llvm::Value* llvmval);
  void registerLlvmValforFreeVar(mir::valueptr mirval, llvm::Value* llvmval);

  void registerLlvmVal(std::shared_ptr<mir::Argument> mirval, llvm::Value* llvmval);

  llvm::Value* getLlvmVal(mir::valueptr mirval);
  llvm::Value* getLlvmValForFcallArgs(mir::valueptr mirval);
  std::vector<llvm::Value*> makeFcallArgs(llvm::Type* ft, std::list<mir::valueptr> const& args);

  std::unordered_map<mir::valueptr, llvm::Value*> mir_to_llvm;

  std::unordered_map<mir::valueptr, llvm::Value*> mirfv_to_llvm;
  std::unordered_map<mir::valueptr, llvm::Value*> memobj_to_llvm;
  std::queue<llvm::Value*> memobjqueue;
  std::unordered_map<mir::valueptr, llvm::Value*> fun_to_selfval;
  std::unordered_map<mir::valueptr, llvm::Value*> fun_to_selfptr;

  std::unordered_map<std::shared_ptr<mir::Argument>, llvm::Value*> mirarg_to_llvm;

  LLVMGenerator& G;  // NOLINT
  const funobjmap* funobj_map;
  bool isglobal;
  bool context_hasself;
  minst::Function* recursivefn_ptr;
  llvm::Value* getFunForFcall(minst::Fcall const& i);
  llvm::Value* getDirFun(minst::Fcall const& i);
  llvm::Value* getClsFun(minst::Fcall const& i);
  llvm::Value* getExtFun(minst::Fcall const& i);
  llvm::Value* popMemobjInContext();
  llvm::FunctionType* createFunctionType(
      minst::Function const& i, bool hascapture,
      std::optional<std::shared_ptr<FunObjTree>> const& memobjtype);
  llvm::FunctionType* createDspFnType(minst::Function const& i, bool hascapture,
                                      std::optional<std::shared_ptr<FunObjTree>> const& memobjtype);

  llvm::Function* createFunction(llvm::FunctionType* type, minst::Function& i);
  void addArgstoMap(llvm::Function* f, minst::Function& i, bool hascapture, bool hasmemobj);

  void setFvsToMap(minst::Function& i, llvm::Value* clsarg);
  void setMemObjsToMap(mir::valueptr fun, llvm::Value* memarg);
  llvm::Value* createAllocation(bool isglobal, llvm::Type* type, llvm::Value* array_size,
                                const llvm::Twine& name);
  llvm::Value* createIfBody(mir::blockptr& block);
  // const static std::unordered_map<ast::OpId, std::string> opid_to_ffi;

  mir::valueptr instance_holder = nullptr;
  llvm::Value* getConstant(const mir::Constants& val);
  template <class T>
  mir::valueptr getValPtr(T* ptr) {
    assert(ptr == &mir::getInstRef<T>(instance_holder));
    return instance_holder;
  }
};
}  // namespace mimium