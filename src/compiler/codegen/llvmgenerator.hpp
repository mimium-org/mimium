/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "basic/mir.hpp"
#include "compiler/codegen/llvm_header.hpp"
#include "compiler/collect_memoryobjs.hpp"
#include "compiler/ffi.hpp"
#include "runtime/runtime_defs.hpp"

namespace mimium {
struct LLVMBuiltin;
struct CodeGenVisitor;
struct TypeConverter;
class LLVMGenerator {
  friend struct CodeGenVisitor;

 public:
  explicit LLVMGenerator(llvm::LLVMContext& ctx);
  ~LLVMGenerator();
  void generateCode(mir::blockptr mir, const funobjmap* funobjs);

  llvm::Module& getModule() { return *module; }
  auto moveModule() { return std::move(module); }
  void init(std::string filename);
  void setDataLayout(const llvm::DataLayout& dl);
  void reset(std::string filename);

  void outputToStream(llvm::raw_ostream& ostream);
  static void dumpvar(llvm::Value* v);
  static void dumpvar(llvm::Type* v);

 private:
  llvm::LLVMContext& ctx;
  llvm::Function* curfunc;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  llvm::BasicBlock* mainentry;
  llvm::BasicBlock* currentblock;
  std::unique_ptr<TypeConverter> typeconverter;
  std::shared_ptr<CodeGenVisitor> codegenvisitor;

  llvm::Type* getType(types::Value& type);

  llvm::Type* getClosureToFunType(types::Value& type);
  const std::unordered_map<std::string, llvm::Type*> runtime_fun_names;

  struct {
    llvm::Value* capptr = nullptr;
    llvm::Value* memobjptr = nullptr;
    int in_numchs =0;
    int out_numchs =0;
  } runtime_dspfninfo;

  void switchToMainFun();
  void preprocess();
  llvm::Function* getForeignFunction(const std::string& name);
  llvm::Function* getRuntimeFunction(const std::string& name);
  llvm::Function* getFunction(const std::string& name, llvm::Type* type);

  void createMiscDeclarations();
  void createRuntimeSetDspFn(llvm::Type* memobjtype);
  void checkDspFunctionType(minst::Function const& i);
  static std::optional<int> getDspFnChannelNumForType(types::Value const& t);
  void createMainFun();
  void createTaskRegister(bool isclosure);
  void createNewBasicBlock(std::string name, llvm::Function* f);
  void visitInstructions(mir::valueptr inst, bool isglobal);
  void setBB(llvm::BasicBlock* newblock);
  void dropAllReferences();

  llvm::Value* getRuntimeInstance();

  auto getDoubleTy() { return llvm::Type::getDoubleTy(ctx); }
  auto geti8PtrTy() { return builder->getInt8PtrTy(); }
  auto geti64Ty() { return builder->getInt64Ty(); }

  auto getConstInt(int v, const int bitsize = 64) {
    return llvm::ConstantInt::get(llvm::IntegerType::get(ctx, bitsize), llvm::APInt(bitsize, v));
  }
  auto getConstDouble(double v) { return llvm::ConstantFP::get(builder->getDoubleTy(), v); }

  auto getZero(const int bitsize = 64) { return getConstInt(0, bitsize); }

};

}  // namespace mimium