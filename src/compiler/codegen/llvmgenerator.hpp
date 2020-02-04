/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/ast.hpp"
#include "basic/helper_functions.hpp"
#include "basic/mir.hpp"
#include "compiler/closure_convert.hpp"
#include "compiler/collect_memoryobjs.hpp"
#include "compiler/ffi.hpp"
#include "compiler/codegen/llvm_header.hpp"

#include "compiler/codegen/typeconverter.hpp"
#include "compiler/codegen/codegen_visitor.hpp"

namespace mimium {
struct LLVMBuiltin;
struct CodeGenVisitor;
class LLVMGenerator{
friend struct CodeGenVisitor;

 private:
  llvm::LLVMContext& ctx;
  llvm::Function* curfunc;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  llvm::BasicBlock* mainentry;
  llvm::BasicBlock* currentblock;

  TypeEnv& typeenv;
  TypeConverter typeconverter;
  std::shared_ptr<CodeGenVisitor> codegenvisitor;
  ClosureConverter& cc;
  MemoryObjsCollector& memobjcoll;

  llvm::FunctionCallee addtask;
  llvm::FunctionCallee addtask_cls;
  llvm::Type* getType(types::Value& type);
  // search from typeenv
  llvm::Type* getType(const std::string& name);
  llvm::Type* getClosureToFunType(types::Value& type);

  using namemaptype = std::unordered_map<std::string, llvm::Value*>;
  std::unordered_map<llvm::Function*, std::shared_ptr<namemaptype>>
      variable_map;
      
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
  void visitInstructions(Instructions& inst, bool isglobal);

  void dropAllReferences();

 public:
  LLVMGenerator(llvm::LLVMContext& ctx, TypeEnv& typeenv,ClosureConverter& cc,MemoryObjsCollector& memobjcoll);

  llvm::Module& getModule() { return *module; }
  auto moveModule() { return std::move(module); }
  ~LLVMGenerator();
  void init(std::string filename);
  void setDataLayout(const llvm::DataLayout& dl);
  void reset(std::string filename);
  void setBB(llvm::BasicBlock* newblock);
  void generateCode(std::shared_ptr<MIRblock> mir);

  void outputToStream(llvm::raw_ostream& ostream);
  void dumpvars();
  
};

}  // namespace mimium