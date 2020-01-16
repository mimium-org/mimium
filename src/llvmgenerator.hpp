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


#include "ffi.hpp"
#include "jit_engine.hpp"

namespace mimium {
struct LLVMBuiltin;

class LLVMGenerator : public std::enable_shared_from_this<LLVMGenerator> {
 private:
  // std::string filename;
  [[maybe_unused]]bool isjit;
  auto getType(types::Value& type) -> llvm::Type*;
  auto getRawStructType( types::Struct& type) -> llvm::Type*;
  void preprocess();
  void createMiscDeclarations();
  void createMainFun();
  void createFcall(std::shared_ptr<FcallInst> i,std::vector<llvm::Value*>& args);
  void createTaskRegister(bool isclosure);
  llvm::Value* createAllocation(bool isglobal,llvm::Type* type,llvm::Value *ArraySize,const llvm::Twine& name);
  bool createStoreOw(std::string varname,llvm::Value* val_to_store);
  void createAddTaskFn(std::shared_ptr<FcallInst> i, bool isclosure,bool isglobal);

  void visitInstructions(const Instructions& inst, bool isglobal);

  void dropAllReferences();
  std::unordered_map<std::string, llvm::Type*> typemap;
  llvm::FunctionCallee addtask;
  llvm::FunctionCallee addtask_cls;

  [[maybe_unused]] unsigned int taskfn_typeid;
  std::vector<types::Value> tasktype_list;
  int struct_index=0;
  std::unordered_map<std::string,types::Struct> structtype_map;
  void initJit();
  llvm::Error doJit(size_t opt_level = 1);

  llvm::Type* getOrCreateTimeStruct(types::Time& t);
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
  void* execute();
  void outputToStream(llvm::raw_ostream& ostream);
  llvm::orc::MimiumJIT& getJitEngine(){return *jitengine;}
  auto& getTaskInfoList(){return tasktype_list;}
};

// struct InstructionVisitor{
//     void operator()(NumberInst)
// };
}  // namespace mimium