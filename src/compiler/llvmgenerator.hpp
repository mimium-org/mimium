#pragma once
#include <algorithm>
#include <memory>
#include <unordered_map>

#include "basic/ast.hpp"
#include "basic/helper_functions.hpp"
#include "basic/mir.hpp"
#include "basic/variant_visitor_helper.hpp"
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
  llvm::Function* curfunc;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  llvm::BasicBlock* mainentry;
  llvm::BasicBlock* currentblock;
  TypeEnv& typeenv;

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

  void visitInstructions(Instructions& inst, bool isglobal);

  void dropAllReferences();
  llvm::FunctionCallee addtask;
  llvm::FunctionCallee addtask_cls;

  [[maybe_unused]] unsigned int taskfn_typeid;
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
  struct TypeConverter {
    explicit TypeConverter(llvm::IRBuilder<>& b, llvm::Module& m)
        : builder(b), module(m), tmpname(""){};
    llvm::IRBuilder<>& builder;
    llvm::Module& module;
    std::string tmpname;
    std::unordered_map<std::string, llvm::Type*> aliasmap;
    static void error() { throw std::logic_error("Invalid Type"); }
    llvm::Type* operator()(std::monostate) const {
      error();
      return nullptr;
    }
    llvm::Type* operator()(types::None) {
      error();
      return nullptr;
    }
    llvm::Type* operator()(const types::TypeVar& v) {
      throw std::logic_error("Type inference failed");
      return nullptr;
    }
    llvm::Type* operator()(types::Void) const { return builder.getVoidTy(); }
    llvm::Type* operator()(types::Float) const { return builder.getDoubleTy(); }
    llvm::Type* operator()(types::String) const {
      return builder.getInt8PtrTy();
    }
    llvm::Type* operator()(const types::Ref& r) {
      llvm::Type* res = llvm::PointerType::get(std::visit(*this, r.val), 0);
      return res;
    }
    llvm::Type* operator()(const types::Pointer& r) {
      llvm::Type* res = llvm::PointerType::get(std::visit(*this, r.val), 0);
      return res;
    }
    llvm::Type* operator()(const types::Function& f) {
      std::vector<llvm::Type*> ar;
      for (auto& a : f.arg_types) {
        ar.push_back(std::visit(*this, a));
      }
      llvm::Type* ret = std::visit(*this, f.ret_type);
      if (std::holds_alternative<recursive_wrapper<types::Function>>(
              f.ret_type)) {
        ret = llvm::PointerType::get(ret, 0);
      }
      return (llvm::Type*)llvm::FunctionType::get(ret, ar, false);
    }
    llvm::Type* operator()(const types::Closure& c) {
      auto name = consumeAlias();
      auto* capturetype =std::visit(*this, c.captures);
      auto* fty = (*this)(c.fun);
      return llvm::StructType::create(
          builder.getContext(), {fty, capturetype}, name, false);
    }
    llvm::Type* operator()(const types::Array& a) {
      return (llvm::Type*)llvm::ArrayType::get(std::visit(*this, a.elem_type),
                                               a.size);
    }
    llvm::Type* operator()(const types::Struct& s) {
      std::vector<llvm::Type*> ar;
      llvm::Type* res;
      auto t = static_cast<const types::Tuple>(s);
      for (const auto& a : t.arg_types) {
        ar.push_back(std::visit(*this, a));
      }
      if (tmpname.empty()) {
        res = llvm::StructType::get(builder.getContext(), ar);
      } else {
        auto n = consumeAlias();
        res = tryGetNamedType(n);
        if (res == nullptr) {
          res = llvm::StructType::create(builder.getContext(), ar, n, false);
        }
      }
      return res;
    }
    llvm::Type* operator()(const types::Tuple& t) {
      std::vector<llvm::Type*> ar;
      llvm::Type* res;
      for (const auto& a : t.arg_types) {
        ar.push_back(std::visit(*this, a));
      }
      if (tmpname.empty()) {
        res = llvm::StructType::get(builder.getContext(), ar, false);
      } else {
        auto n = consumeAlias();
        res = tryGetNamedType(n);
        if (res == nullptr) {
          res = llvm::StructType::create(builder.getContext(), ar, n, false);
        }
      }
      return res;
    }
    llvm::Type* operator()(const types::Time& t) {
      llvm::Type* res;
      if (tmpname.empty()) {
        res = llvm::StructType::get(
            builder.getContext(),
            {builder.getDoubleTy(), std::visit(*this, t.val)});
      } else {
        auto n = consumeAlias();
        res = tryGetNamedType(n);
        if (res == nullptr) {
          res = llvm::StructType::create(
              builder.getContext(),
              {builder.getDoubleTy(), std::visit(*this, t.val)}, n);
        }
      }
      return res;
    }
    llvm::Type* operator()(const types::Alias& a) {
      auto it = aliasmap.find(a.name);
      llvm::Type* res;
      if (it == aliasmap.end()) {
        tmpname = a.name;
        res = std::visit(*this, a.target);
        aliasmap.emplace(a.name, res);
      } else {
        res = it->second;
      }
      return res;
    }

   private:
    std::string consumeAlias() {
      std::string t = tmpname;
      tmpname = "";
      return t;
    }
    llvm::Type* tryGetNamedType(std::string name) {
      return module.getTypeByName(name);
    }
  } typeconverter;
};

// struct InstructionVisitor{
//     void operator()(NumberInst)
// };
}  // namespace mimium