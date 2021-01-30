/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "basic/mir.hpp"
namespace llvm {
class LLVMContext;
class Module;
class DataLayout;
class raw_ostream;
class Value;
class Type;
class PointerType;
class BasicBlock;
class ArrayType;
class Function;
class ConstantInt;

class IRBuilderBase;
}  // namespace llvm

namespace mimium {
struct FunObjTree;
using funobjmap = std::unordered_map<mir::valueptr, std::shared_ptr<FunObjTree>>;

class IRBuilderPrivate;
struct LLVMBuiltin;
struct CodeGenVisitor;
struct TypeConverter;

namespace minst = mir::instruction;
class MIMIUM_DLL_PUBLIC LLVMGenerator {
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
  std::unique_ptr<llvm::IRBuilderBase> builder;
  llvm::BasicBlock* mainentry;
  llvm::BasicBlock* currentblock;
  std::unique_ptr<TypeConverter> typeconverter;
  std::shared_ptr<CodeGenVisitor> codegenvisitor;

  llvm::Type* getType(types::Value const& type);
  // Used for getting Arraytype which is not pointer of elementtype
  llvm::ArrayType* getArrayType(types::Value const& type);

  llvm::Type* getClosureToFunType(types::Value& type);
  const std::unordered_map<std::string, llvm::Type*> runtime_fun_names;

  struct {
   public:
    llvm::Value* capptr = nullptr;
    llvm::Value* memobjptr = nullptr;
    int in_numchs = 0;
    int out_numchs = 0;
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

  llvm::Type* getDoubleTy();
  llvm::PointerType* geti8PtrTy();
  llvm::Type* geti64Ty();
  llvm::Value* getConstInt(int v, int bitsize = 64);
  llvm::Value* getConstDouble(double v);
  llvm::Value* getZero(int bitsize = 64);
};

}  // namespace mimium