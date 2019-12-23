#include "llvmgenerator.hpp"

#include <sys/types.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>

#include "jit_engine.hpp"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Error.h"
#include "type.hpp"
namespace mimium {
LLVMGenerator::LLVMGenerator(std::string filename, bool i_isjit)
    : mainentry(nullptr),
      isjit(i_isjit),
      currentblock(nullptr),
      jitengine(std::make_unique<llvm::orc::MimiumJIT>()),
      ctx(jitengine->getContext()) {
  init(filename);
}
void LLVMGenerator::init(std::string filename) {
  builder = std::make_unique<llvm::IRBuilder<>>(ctx);
  module = std::make_unique<llvm::Module>(filename, ctx);
  builtinfn = std::make_shared<LLVMBuiltin>();
  module->setDataLayout(jitengine->getDataLayout());
}
void LLVMGenerator::reset(std::string filename) {
  dropAllReferences();
  init(filename);
}

void LLVMGenerator::initJit() {}

// LLVMGenerator::LLVMGenerator(llvm::LLVMContext& _ctx,std::string _filename){
//     // ctx.reset();
//     // ctx = std::move(&_ctx);
// }

LLVMGenerator::~LLVMGenerator() { dropAllReferences(); }
void LLVMGenerator::dropAllReferences() {
  namemap.clear();
  auto& flist = module->getFunctionList();
  auto f = flist.begin();
  while (!flist.empty()) {
    auto& bbs = f->getBasicBlockList();
    while (!bbs.empty()) {
      auto bb = bbs.begin();
      auto inst = bb->begin();
      while (!bb->getInstList().empty()) {
        inst->replaceAllUsesWith(llvm::UndefValue::get(inst->getType()));
        // inst->dropAllReferences();
        inst = inst->eraseFromParent();
      }
      bb->dropAllReferences();
      bb = bb->eraseFromParent();
      bb = bbs.begin();
    }
    f->dropAllReferences();
    flist.erase(f);
    f = flist.begin();
  }
}

auto LLVMGenerator::getRawStructType(const types::Value& type) -> llvm::Type* {
  types::Struct s = std::get<recursive_wrapper<types::Struct>>(type);
  std::vector<llvm::Type*> field;
  for (auto& a : s.arg_types) {
    field.push_back(getType(a));
  }

  llvm::Type* structtype = llvm::StructType::create(ctx, field, "fvtype");
  return structtype;
}
auto LLVMGenerator::getType(const types::Value& type) -> llvm::Type* {
  return std::visit(
      overloaded{[this](const types::Float& /*f*/) {
                   return llvm::Type::getDoubleTy(ctx);
                 },
                 [this](const recursive_wrapper<types::Function>& rf) {
                   auto f = types::Function(rf);
                   std::vector<llvm::Type*> args;
                   auto* rettype = getType(f.getReturnType());
                   for (auto& a : f.getArgTypes()) {
                     auto* atype = getType(a);
                     args.push_back(atype);
                   }
                   return llvm::cast<llvm::Type>(
                       llvm::FunctionType::get(rettype, args, false));
                 },
                 [this](const recursive_wrapper<types::Struct>& rs) {
                   auto s = types::Struct(rs);
                   std::vector<llvm::Type*> field;
                   for (auto& a : s.arg_types) {
                     field.push_back(llvm::PointerType::get(getType(a), 0));
                   }
                   llvm::Type* structtype = llvm::PointerType::get(
                       llvm::StructType::create(ctx, field, "fvtype"), 0);
                   return structtype;
                 },
                 [this](const types::Void& /* t */) {
                   return llvm::Type::getVoidTy(ctx);
                 },
                 [this](auto& t) {  // NOLINT
                   throw std::logic_error("invalid type");
                   return llvm::Type::getVoidTy(ctx);
                   ;
                 }},
      type);
}

void LLVMGenerator::setBB(llvm::BasicBlock* newblock) {
  builder->SetInsertPoint(newblock);
}
void LLVMGenerator::createMainFun() {
  auto* fntype = llvm::FunctionType::get(llvm::Type::getInt64Ty(ctx), false);
  auto* mainfun = llvm::Function::Create(
      fntype, llvm::Function::ExternalLinkage, "__mimium_main", module.get());
  mainfun->setCallingConv(llvm::CallingConv::C);
  using Akind = llvm::Attribute;
  std::vector<llvm::Attribute::AttrKind> attrs = {
      Akind::NoUnwind, Akind::NoInline, Akind::OptimizeNone};
  llvm::AttributeSet aset;
  for (auto& a : attrs) {
    aset = aset.addAttribute(ctx, a);
  }

  mainfun->addAttributes(llvm::AttributeList::FunctionIndex, aset);
  mainentry = llvm::BasicBlock::Create(ctx, "entry", mainfun);
}
void LLVMGenerator::createTaskRegister() {
  // time,address to function(upcasted),argument(currently only double),address
  // to target variable
  auto taskfntype = llvm::FunctionType::get(builder->getDoubleTy(),
                                            {builder->getDoubleTy()}, false);

  llvm::ArrayRef<llvm::Type*> argtypes = {
      builder->getDoubleTy(), llvm::PointerType::get(taskfntype, 0),
      builder->getDoubleTy(), llvm::Type::getDoublePtrTy(ctx)};
  auto* fntype = llvm::FunctionType::get(builder->getVoidTy(), argtypes, false);
  addtask = module->getOrInsertFunction("addTask", fntype);
  auto addtaskfun = llvm::cast<llvm::Function>(addtask.getCallee());
  addtaskfun->setCallingConv(llvm::CallingConv::C);
  using Akind = llvm::Attribute;
  std::vector<llvm::Attribute::AttrKind> attrs = {
      Akind::NoUnwind, Akind::NoInline, Akind::OptimizeNone};
  llvm::AttributeSet aset;
  for (auto& a : attrs) {
    aset = aset.addAttribute(ctx, a);
  }
  addtaskfun->addAttributes(llvm::AttributeList::FunctionIndex, aset);
  typemap.emplace("addTask", fntype);
}

llvm::Type* LLVMGenerator::getOrCreateTimeStruct(types::Value t) {
  llvm::StringRef name =
      "TimeType_" + std::visit([](auto& type) { return type.toString(); }, t);
  llvm::Type* res = module->getTypeByName(name);
  if (res == nullptr) {
    llvm::Type* containtype = std::visit(
        overloaded{[&](types::Float& f) { return builder->getDoubleTy(); },
                   [&](auto& v) { return builder->getVoidTy(); }},
        t);
    llvm::ArrayRef<llvm::Type*> elemtypes = {
        builder->getDoubleTy(), containtype};
    res = llvm::StructType::create(ctx, elemtypes, name);
    typemap.emplace(name, res);
  }
  return res;
}
void LLVMGenerator::preprocess() {
  createTaskRegister();
  createMainFun();
  setBB(mainentry);
}

void LLVMGenerator::generateCode(std::shared_ptr<MIRblock> mir) {
  preprocess();
  for (auto& inst : mir->instructions) {
    visitInstructions(inst);
  }
  if (mainentry->getTerminator() ==
      nullptr) {  // insert empty return if no return
    builder->CreateRet(llvm::ConstantInt::get(builder->getInt64Ty(), 0));
  }
}

void LLVMGenerator::visitInstructions(const Instructions& inst) {
  std::visit(
      overloaded{
          [](auto i) {},
          [&, this](const std::shared_ptr<NumberInst>& i) {
            auto* ptr = builder->CreateAlloca(llvm::Type::getDoubleTy(ctx),
                                              nullptr, "ptr_" + i->lv_name);
            auto* finst =
                llvm::ConstantFP::get(this->ctx, llvm::APFloat(i->val));
            builder->CreateStore(finst, ptr);
            auto* load = builder->CreateLoad(ptr, i->lv_name);
            namemap.emplace("ptr_" + i->lv_name, ptr);
            namemap.emplace(i->lv_name, load);
          },
          [&, this](const std::shared_ptr<TimeInst>& i) {
            auto resname = "ptr_" + i->lv_name;
            types::Time timetype =
                std::get<recursive_wrapper<types::Time>>(i->type);
            auto strtype = getOrCreateTimeStruct(timetype.val);
            typemap.emplace(resname, strtype);
            auto* ptr = builder->CreateAlloca(strtype, nullptr, resname);
            auto* timepos = builder->CreateStructGEP(strtype, ptr, 0);
            auto* valpos = builder->CreateStructGEP(strtype, ptr, 1);
            namemap[i->time]->dump();
            timepos->dump();
            valpos->dump();
            auto* time =
                builder->CreateFPToUI(namemap[i->time], builder->getDoubleTy());
            builder->CreateStore(time, timepos);

            builder->CreateStore(namemap.find(i->val)->second, valpos);
            namemap.emplace(resname, ptr);
          },
          [&, this](const std::shared_ptr<OpInst>& i) {
            llvm::Value* ptr;
            auto* lhs = namemap[i->lhs];
            auto* rhs = namemap[i->rhs];
            switch (i->getOPid()) {
              case ADD:
                ptr = builder->CreateFAdd(lhs, rhs, i->lv_name);
                break;
              case SUB:
                ptr = builder->CreateFSub(lhs, rhs, i->lv_name);
                break;
              case MUL:
                ptr = builder->CreateFMul(lhs, rhs, i->lv_name);
                break;
              case DIV:
                ptr = builder->CreateFDiv(lhs, rhs, i->lv_name);
                break;
              default:
                break;
            }
            namemap.emplace(i->lv_name, ptr);
          },
          [&, this](const std::shared_ptr<FunInst>& i) {
            bool hasfv = !i->freevariables.empty();
            auto* ft =
                llvm::cast<llvm::FunctionType>(getType(i->type));  // NOLINT
            llvm::Function* f = llvm::Function::Create(
                ft, llvm::Function::ExternalLinkage, i->lv_name, module.get());
            auto f_it = f->args().begin();

            std::for_each(i->args.begin(), i->args.end(),
                          [&](std::string s) { (f_it++)->setName(s); });
            if (hasfv) {
              auto it = f->args().end();
              (--it)->setName("clsarg_" + i->lv_name);
            }
            namemap.emplace(i->lv_name, f);

            auto* bb = llvm::BasicBlock::Create(ctx, "entry", f);
            builder->SetInsertPoint(bb);
            currentblock = bb;
            f_it = f->args().begin();
            std::for_each(i->args.begin(), i->args.end(),
                          [&](std::string s) { namemap.emplace(s, f_it++); });

            auto arg_end = f->arg_end();
            llvm::Value* lastarg = --arg_end;
            for (int id = 0; id < i->freevariables.size(); id++) {
              std::string newname = "fv_" + i->freevariables[id].name;
              llvm::Value* gep = builder->CreateStructGEP(lastarg, id, "fv");
              llvm::Value* ptrload =
                  builder->CreateLoad(gep, "ptrto_" + newname);
              llvm::Value* valload = builder->CreateLoad(ptrload, newname);
              namemap.try_emplace(newname, valload);
            }
            for (auto& cinsts : i->body->instructions) {
              visitInstructions(cinsts);
            }
            setBB(mainentry);
            currentblock = mainentry;
          },
          [&, this](const std::shared_ptr<MakeClosureInst>& i) {
            auto it = llvm::cast<llvm::Function>(namemap[i->fname])  // NOLINT
                          ->arg_end();
            auto ptrtype =
                llvm::cast<llvm::PointerType>((--it)->getType());  // NOLINT
            llvm::Type* strtype = ptrtype->getElementType();
            llvm::Value* cap_size =
                builder->CreateAlloca(strtype, nullptr, i->lv_name);
            int idx = 0;
            for (auto& cap : i->captures) {
              llvm::Value* gep =
                  builder->CreateStructGEP(strtype, cap_size, idx++, "");
              builder->CreateStore(namemap["ptr_" + cap.name], gep);
            }
            namemap.emplace(i->lv_name, cap_size);
          },
          [&, this](const std::shared_ptr<FcallInst>& i) {
            llvm::Value* res;
            std::vector<llvm::Value*> args;
            std::for_each(i->args.begin(), i->args.end(),
                          [&](auto& a) { args.emplace_back(namemap[a]); });
            if (i->ftype == CLOSURE) {
              args.emplace_back(namemap[i->fname + "_cls"]);
            }
            if (i->istimed) {  // if arguments is timed value, call addTask
                               // function,
              auto tvptr = namemap["ptr_" + i->args[0]];
              auto timeptr = builder->CreateStructGEP(
                  typemap["ptr_" + i->args[0]], tvptr, 0, "");
              auto valptr = builder->CreateStructGEP(
                  typemap["ptr_" + i->args[0]], tvptr, 1, "");
              auto timeval = builder->CreateLoad(timeptr);
              auto val = builder->CreateLoad(valptr);
              auto ptrtofn = namemap[i->fname];
              auto taskfntype = llvm::FunctionType::get(
                  builder->getDoubleTy(), {builder->getDoubleTy()}, false);

              auto addresstofn = builder->CreatePointerCast(
                  ptrtofn, llvm::PointerType::get(taskfntype, 0));
              // time,address to fun, arg(double), ptrtotarget,
              llvm::ArrayRef<llvm::Value*> args = {
                  timeval, addresstofn, val, namemap["ptr_" + i->lv_name]};
              // auto rettype
              // =llvm::cast<llvm::FunctionType>(typemap["addTask"])->getReturnType();
              for(const auto& a:args){a->dump();};
              builder->CreateCall(addtask, args);
            }else{
            if (LLVMBuiltin::isBuiltin(i->fname)) {
              auto it = LLVMBuiltin::builtin_fntable.find(i->fname);
              builtintype fn = it->second;
              res = fn(args, i->lv_name, this->shared_from_this());
            } else {
              llvm::Function* fun = module->getFunction(i->fname);
              if (fun == nullptr) {
                throw std::logic_error("function could not be referenced");
              }
              res = builder->CreateCall(fun, args, i->lv_name);
            }
            namemap.emplace(i->lv_name, res);
            }
          },
          [&, this](const std::shared_ptr<ReturnInst>& i) {
            builder->CreateRet(namemap[i->val]);
          }},
      inst);
}

llvm::Error LLVMGenerator::doJit(const size_t opt_level) {
  return jitengine->addModule(
      std::move(module));  // do JIT compilation for module
}
int LLVMGenerator::execute() {
  llvm::Error err = doJit();
  Logger::debug_log(err, Logger::ERROR);
  auto mainfun = jitengine->lookup("__mimium_main");
  Logger::debug_log(mainfun, Logger::ERROR);
  auto fnptr =
      llvm::jitTargetAddressToPointer<int64_t (*)()>(mainfun->getAddress());
    
  int64_t res = fnptr();
  return res;
}
void LLVMGenerator::outputToStream(llvm::raw_ostream& stream) {
  module->print(stream, nullptr, false, true);
}

}  // namespace mimium