#pragma once
#include <algorithm>
#include <memory>
#include <unordered_map>

#include "alphaconvert_visitor.hpp"
#include "ast.hpp"
#include "closure_convert.hpp"
#include "knormalize_visitor.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"


#include "llvm_builtin_functions.hpp"
#include "jit_engine.hpp"

namespace mimium {
class LLVMBuiltin;

class LLVMGenerator : public std::enable_shared_from_this<LLVMGenerator> {
 private:
  // std::string filename;
  std::shared_ptr<LLVMBuiltin> builtinfn;
  [[maybe_unused]]bool isjit;
  auto getType(const types::Value& type) -> llvm::Type*;
  auto getRawStructType(const types::Value& type) -> llvm::Type*;
  void preprocess();
  void createMainFun();
  void createTaskRegister();
  void visitInstructions(const Instructions& inst);
  void dropAllReferences();
  std::unordered_map<std::string, llvm::Type*> typemap;
  llvm::FunctionCallee addtask;

  void initJit();
  llvm::Error doJit(size_t opt_level = 1);

  llvm::Type* getOrCreateTimeStruct(types::Value t);
 public:
   std::unique_ptr<llvm::orc::MimiumJIT> jitengine;
  llvm::LLVMContext& ctx;
  std::unique_ptr<llvm::Function> curfunc;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  std::unordered_map<std::string, llvm::Value*> namemap;
  llvm::BasicBlock* mainentry;
  llvm::BasicBlock* currentblock;
  explicit LLVMGenerator(std::string filename,bool i_isjit=true);
  // explicit LLVMGenerator(llvm::LLVMContext& _cts,std::string filename);

  ~LLVMGenerator();
  void init(std::string filename);
  void reset(std::string filename);

  void setBB(llvm::BasicBlock* newblock);

  void generateCode(std::shared_ptr<MIRblock> mir);
  int execute();
  void outputToStream(llvm::raw_ostream& ostream);
  llvm::orc::MimiumJIT& getJitEngine(){return *jitengine;}
};

// struct InstructionVisitor{
//     void operator()(NumberInst)
// };
}  // namespace mimium