#pragma once
#include <algorithm>
#include <memory>
#include <unordered_map>

#include "basic/ast.hpp"
#include "basic/helper_functions.hpp"
#include "basic/mir.hpp"
#include "compiler/ffi.hpp"
#include "compiler/codegen/llvm_header.hpp"

#include "compiler/codegen/typeconverter.hpp"

#include "runtime/jit_engine.hpp"

namespace mimium {
struct LLVMBuiltin;

class LLVMGenerator : public std::enable_shared_from_this<LLVMGenerator> {
 private:
  // std::string filename;
  [[maybe_unused]] bool isjit;
  llvm::LLVMContext& ctx;
  std::unique_ptr<llvm::orc::MimiumJIT> jitengine;
  llvm::Function* curfunc;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  llvm::BasicBlock* mainentry;
  llvm::BasicBlock* currentblock;
  TypeEnv& typeenv;
  TypeConverter typeconverter;

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
  // auto getRawTupleType(types::Tuple& type) -> llvm::Type*;
  // auto getRawStructType(types::Struct& type) -> llvm::Type*;

  void preprocess();
  llvm::Function* getForeignFunction(std::string name);

  void createMiscDeclarations();
  void createMainFun();
  void createTaskRegister(bool isclosure);

  void createNewBasicBlock(std::string name, llvm::Function* f);
  void visitInstructions(Instructions& inst, bool isglobal);

  void dropAllReferences();
  llvm::FunctionCallee addtask;
  llvm::FunctionCallee addtask_cls;
  void initJit();
  llvm::Error doJit(size_t opt_level = 1);

  llvm::Type* getOrCreateTimeStruct(types::Time& t);

  struct CodeGenVisitor {
    CodeGenVisitor(LLVMGenerator& g) : G(g){};
    void operator()(NumberInst& i);
    void operator()(AllocaInst& i);
    void operator()(RefInst& i);
    void operator()(AssignInst& i);
    void operator()(TimeInst& i);
    void operator()(OpInst& i);
    void operator()(FunInst& i);
    void operator()(FcallInst& i);
    void operator()(MakeClosureInst& i);
    void operator()(ArrayInst& i);
    void operator()(ArrayAccessInst& i);
    void operator()(IfInst& i);
    void operator()(ReturnInst& i);
    bool isglobal;

   private:
    LLVMGenerator& G;
    llvm::Value* getDirFun(FcallInst& i);
    llvm::Value* getClsFun(FcallInst& i);
    llvm::Value* getExtFun(FcallInst& i);

    llvm::Function* createFunction(llvm::FunctionType* type,FunInst& i);
    void addArgstoMap(llvm::Function* f,FunInst& i);

    void setFvsToMap(FunInst& i, llvm::Function* f);
    llvm::Value* createAllocation(bool isglobal, llvm::Type* type,
                                  llvm::Value* ArraySize,
                                  const llvm::Twine& name);
    bool createStoreOw(std::string varname, llvm::Value* val_to_store);
    void createAddTaskFn(FcallInst& i, bool isclosure, bool isglobal);
  } codegenvisitor;

 public:
  LLVMGenerator(llvm::LLVMContext& ctx, TypeEnv& typeenv);

  llvm::Module& getModule() { return *module; }
  auto moveModule() { return std::move(module); }
  ~LLVMGenerator();
  void init(std::string filename);
  void setDataLayout();
  void setDataLayout(const llvm::DataLayout& dl);

  void reset(std::string filename);

  void setBB(llvm::BasicBlock* newblock);

  void generateCode(std::shared_ptr<MIRblock> mir);
  void* execute();
  void outputToStream(llvm::raw_ostream& ostream);
  llvm::orc::MimiumJIT& getJitEngine() { return *jitengine; }
  void dumpvars() {
    for (auto& [f, map] : variable_map) {
      llvm::errs() << f->getName() << ":\n";
      for (auto& [key, val] : *map) {
        llvm::errs() << "   " << key << " :  " << val->getName() << "\n";
      }
    }
  }
};

// struct InstructionVisitor{
//     void operator()(NumberInst)
// };
}  // namespace mimium