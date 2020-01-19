#pragma once
#include <algorithm>
#include <memory>
#include <unordered_map>

#include "basic/ast.hpp"
#include "basic/mir.hpp"
#include "compiler/ffi.hpp"
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
#include "runtime/jit_engine.hpp"

namespace mimium {
struct LLVMBuiltin;

class LLVMGenerator : public std::enable_shared_from_this<LLVMGenerator> {
 private:
  // std::string filename;
  [[maybe_unused]] bool isjit;
  llvm::LLVMContext& ctx;
  std::unique_ptr<llvm::orc::MimiumJIT> jitengine;
  auto getType(types::Value& type) -> llvm::Type*;
  // auto getRawTupleType(types::Tuple& type) -> llvm::Type*;
  // auto getRawStructType(types::Struct& type) -> llvm::Type*;

  void preprocess();
  llvm::Function* getForeignFunction(std::string name);

  void createMiscDeclarations();
  void createMainFun();
  void createFcall(std::shared_ptr<FcallInst> i,
                   std::vector<llvm::Value*>& args);
  void createTaskRegister(bool isclosure);
  llvm::Value* createAllocation(bool isglobal, llvm::Type* type,
                                llvm::Value* ArraySize,
                                const llvm::Twine& name);
  bool createStoreOw(std::string varname, llvm::Value* val_to_store);
  void createAddTaskFn(std::shared_ptr<FcallInst> i, bool isclosure,
                       bool isglobal);

  void visitInstructions(const Instructions& inst, bool isglobal);

  void dropAllReferences();
  std::unordered_map<std::string, llvm::Type*> typemap;
  llvm::FunctionCallee addtask;
  llvm::FunctionCallee addtask_cls;

  [[maybe_unused]] unsigned int taskfn_typeid;
  std::vector<types::Value> tasktype_list;
  int struct_index = 0;
  std::unordered_map<std::string, types::Tuple> structtype_map;
  void initJit();
  llvm::Error doJit(size_t opt_level = 1);

  llvm::Type* getOrCreateTimeStruct(types::Time& t);

 public:
  std::unique_ptr<llvm::Function> curfunc;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  std::unordered_map<std::string, llvm::Value*> namemap;
  llvm::BasicBlock* mainentry;
  llvm::BasicBlock* currentblock;
  LLVMGenerator(llvm::LLVMContext& ctx);
  // old
  // explicit LLVMGenerator(std::string filename,bool i_isjit=true);
  // explicit LLVMGenerator(llvm::LLVMContext& _cts,std::string filename);
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
  auto& getTaskInfoList() { return tasktype_list; }
  struct TypeConverter {
    explicit TypeConverter(llvm::IRBuilder<>& b, llvm::Module& m)
        : builder(b), module(m){};
    llvm::IRBuilder<>& builder;
    llvm::Module& module;
    static void error() { throw std::logic_error("Invalid Type"); }
    llvm::Type* operator()(std::monostate) const {
      error();
      return nullptr;
    }
    llvm::Type* operator()(types::None) const {
      error();
      return nullptr;
    }
    llvm::Type* operator()(const types::TypeVar& v) const {
      throw std::logic_error("Type inference failed");
      return nullptr;
    }
    llvm::Type* operator()(types::Void) const { return builder.getVoidTy(); }
    llvm::Type* operator()(types::Float) const { return builder.getDoubleTy(); }
    llvm::Type* operator()(types::String) const {
      return builder.getInt8PtrTy();
    }
    llvm::Type* operator()(const types::Ref& r) const {
      if(std::holds_alternative<recursive_wrapper<types::Function>>(r.val)){
      return (llvm::Type*)llvm::ArrayType::get(std::visit(*this, r.val), 0);
      }
      return (llvm::Type*)llvm::PointerType::get(std::visit(*this, r.val), 0);
      
      }
    llvm::Type* operator()(const types::Pointer& r) const {
      return (llvm::Type*)llvm::PointerType::get(std::visit(*this, r.val), 0);
    }
    llvm::Type* operator()(const types::Function& f) const {
      std::vector<llvm::Type*> ar;
      for (auto& a : f.arg_types) {
        ar.push_back(std::visit(*this, a));
      }
      llvm::Type* res = std::visit(*this, f.ret_type);
      if(std::holds_alternative<recursive_wrapper<types::Function>>(f.ret_type)){
          res = llvm::PointerType::get(res,0);
      }
      return (llvm::Type*)llvm::FunctionType::get(res,
                                                  ar, false);
    }
    llvm::Type* operator()(const types::Closure& c) const {
      return std::visit(*this, c.fun);
    }
    llvm::Type* operator()(const types::Array& a) const {
      return (llvm::Type*)llvm::ArrayType::get(std::visit(*this, a.elem_type),
                                               a.size);
    }
    llvm::Type* operator()(const types::Struct& s) const {
      llvm::StringRef name = types::toString(s);
      std::vector<llvm::Type*> ar;
      llvm::Type* res;
      auto* defined = module.getTypeByName(name);
      if(defined==nullptr){
      auto t = static_cast<const types::Tuple>(s);
      for (const auto& a : t.arg_types) {
        ar.push_back(std::visit(*this, a));
      }
      res = (llvm::Type*)llvm::StructType::create(builder.getContext(), ar,
                                                   name);
      }else{
        res= defined;
      }
      return res;
    }
    llvm::Type* operator()(const types::Tuple& t) const {
      llvm::StringRef name = types::toString(t);
      auto* defined = module.getTypeByName(name);
            llvm::Type* res;
      std::vector<llvm::Type*> ar;
      if(defined==nullptr){
      for (const auto& a : t.arg_types) {
        ar.push_back(std::visit(*this, a));
      }
      res = (llvm::Type*)llvm::StructType::create(builder.getContext(), ar,
                                                   name);
      }else{
        res=defined;
      }
      return res;
    }
    llvm::Type* operator()(const types::Time& t) const {
      llvm::StringRef name = types::toString(t);
      llvm::Type* res = module.getTypeByName(name);
      if (res == nullptr) {
        res = llvm::StructType::create(
            builder.getContext(),
            {builder.getDoubleTy(), std::visit(*this, t.val)}, name);
      }
      return (llvm::Type*)res;
    }
  };
};

// struct InstructionVisitor{
//     void operator()(NumberInst)
// };
}  // namespace mimium