/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "basic/mir.hpp"
#include "compiler/codegen/llvm_header.hpp"
#include "compiler/ffi.hpp"

namespace mimium {
struct LLVMBuiltin;
struct CodeGenVisitor;
struct TypeConverter;
struct FunObjTree;
using funobjmap = std::unordered_map<std::string, std::shared_ptr<FunObjTree>>;
class LLVMGenerator {
  friend struct CodeGenVisitor;

 private:
  llvm::LLVMContext& ctx;
  llvm::Function* curfunc;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  llvm::BasicBlock* mainentry;
  llvm::BasicBlock* currentblock;
  TypeEnv& typeenv;
  std::unique_ptr<TypeConverter> typeconverter;
  std::shared_ptr<CodeGenVisitor> codegenvisitor;
  std::vector<std::string> overwritten_vars;
  llvm::FunctionCallee addtask;
  llvm::FunctionCallee addtask_cls;
  llvm::Type* getType(types::Value& type);
  // search from typeenv
  llvm::Type* getType(const std::string& name);
  llvm::Type* getClosureToFunType(types::Value& type);

  using namemaptype = std::unordered_map<std::string, llvm::Value*>;
  std::unordered_map<llvm::Function*, std::shared_ptr<namemaptype>> variable_map;

  bool isVarOverWritten(std::string const& name) {
    return std::find(overwritten_vars.begin(), overwritten_vars.end(), name) !=
           overwritten_vars.end();
  }
  void addOverWrittenVar(std::string const& name) {
    if (!isVarOverWritten(name)) { overwritten_vars.emplace_back(name); }
  }
  llvm::Value* findValue(std::string name);
  llvm::Value* tryfindValue(std::string name);
  void switchToMainFun();
  void setValuetoMap(std::string name, llvm::Value* val);
  void preprocess();
  llvm::Function* getForeignFunction(const std::string& name);
  void createMiscDeclarations();
  void createRuntimeSetDspFn();
  void createMainFun();
  void createTaskRegister(bool isclosure);
  void createNewBasicBlock(std::string name, llvm::Function* f);
  llvm::Value* getOrCreateFunctionPointer(llvm::Function* f);
  void visitInstructions(mir::valueptr inst, bool isglobal);

  void dropAllReferences();

 public:
  LLVMGenerator(llvm::LLVMContext& ctx, TypeEnv& typeenv);

  llvm::Module& getModule() { return *module; }
  auto moveModule() { return std::move(module); }
  ~LLVMGenerator();
  void init(std::string filename);
  void setDataLayout(const llvm::DataLayout& dl);
  void reset(std::string filename);
  void setBB(llvm::BasicBlock* newblock);
  void generateCode(mir::blockptr mir, const funobjmap* funobjs);

  void outputToStream(llvm::raw_ostream& ostream);
  void dumpvars();
  static void dumpvar(llvm::Value* v);
  static void dumpvar(llvm::Type* v);
  auto getConstInt(int v, const int bitsize = 64) {
    return llvm::ConstantInt::get(llvm::IntegerType::get(ctx, bitsize), llvm::APInt(bitsize, v));
  }
  auto getConstDouble(double v) { return llvm::ConstantFP::get(builder->getDoubleTy(), v); }

  auto getZero(const int bitsize = 64) { return getConstInt(0, bitsize); }
};

}  // namespace mimium