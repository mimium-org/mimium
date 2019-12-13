#include "llvmgenerator.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <memory>

#include "type.hpp"
namespace mimium {
LLVMGenerator::LLVMGenerator(std::string filename, bool i_isjit)
    : mainentry(nullptr),
      isjit(i_isjit),
      currentblock(nullptr),
      jitengine(nullptr) {
  init(filename);
}
void LLVMGenerator::init(std::string filename) {
  builder = std::make_unique<llvm::IRBuilder<>>(ctx);
  module = std::make_shared<llvm::Module>(filename, ctx);
  builtinfn = std::make_shared<LLVMBuiltin>();
  if (isjit) {
    initJit();
  }
}
void LLVMGenerator::reset(std::string filename) {
  dropAllReferences();
  init(filename);
}

void LLVMGenerator::initJit() {
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();
  jitengine = std::move(llvm::cantFail(llvm::orc::MimiumJIT::createEngine()));
}

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
auto LLVMGenerator::getModule() -> std::shared_ptr<llvm::Module> {
  auto res = module;
  if (module == nullptr) {
    res = std::make_shared<llvm::Module>("null", ctx);
  }
  return res;
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
                   // upcast
                   llvm::Type* res = llvm::FunctionType::get(
                       rettype, args,
                       false);  // what is final parameter isVarArg??
                   return res;
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
                 [this](auto& t) {  // NOLINT
                   throw std::runtime_error("not a function");
                   return llvm::Type::getDoubleTy(ctx);
                   ;
                 }},
      type);
}

void LLVMGenerator::setBB(llvm::BasicBlock* newblock) {
  builder->SetInsertPoint(newblock);
}
void LLVMGenerator::preprocess() {
  auto* fntype = llvm::FunctionType::get(llvm::Type::getInt64Ty(ctx), false);
  auto* mainfun = llvm::Function::Create(
      fntype, llvm::Function::ExternalLinkage, "main", module.get());

  mainentry = llvm::BasicBlock::Create(ctx, "entry", mainfun);
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
          [&, this](const std::shared_ptr<OpInst>& i) {
            llvm::Value* ptr;
            switch (i->getOPid()) {
              case ADD:
                ptr = builder->CreateFAdd(namemap[i->lhs], namemap[i->rhs],
                                          i->lv_name);
                break;
              case SUB:
                ptr = builder->CreateFSub(namemap[i->lhs], namemap[i->rhs],
                                          i->lv_name);
                break;
              case MUL:
                ptr = builder->CreateFMul(namemap[i->lhs], namemap[i->rhs],
                                          i->lv_name);
                break;
              case DIV:
                ptr = builder->CreateFDiv(namemap[i->lhs], namemap[i->rhs],
                                          i->lv_name);
                break;
              default:
                break;
            }
            namemap.emplace(i->lv_name, ptr);
          },
          [&, this](const std::shared_ptr<FunInst>& i) {
            bool hasfv = !i->freevariables.empty();
            auto* ft = static_cast<llvm::FunctionType*>(getType(
                i->type));  // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
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
            auto it = static_cast<llvm::Function*>(namemap[i->fname])  // NOLINT
                          ->arg_end();
            auto ptrtype =
                static_cast<llvm::PointerType*>((--it)->getType());  // NOLINT
            llvm::Type* strtype = ptrtype->getElementType();
            llvm::Value* cap_size =
                builder->CreateAlloca(strtype, nullptr, i->lv_name);
            int idx = 0;
            for (auto& cap : i->captures) {
              llvm::Value* gep =
                  builder->CreateStructGEP(strtype, cap_size, idx, "");
              builder->CreateStore(namemap["ptr_" + cap.name], gep);
              idx++;
            }
            namemap.emplace(i->lv_name, cap_size);
          },
          [&, this](const std::shared_ptr<FcallInst>& i) {
            llvm::Value* res;
            std::vector<llvm::Value*> args;
            for (auto& a : i->args) {
              args.push_back(namemap[a]);
            }
            if (i->ftype == CLOSURE) {
              llvm::Value* cls = namemap[i->fname + "_cls"];
              args.push_back(cls);
            }
            if (LLVMBuiltin::isBuiltin(i->fname)) {
              auto it = LLVMBuiltin::builtin_fntable.find(i->fname);
              builtintype fn = it->second;
              std::vector<llvm::Value*> arg;
              res = fn(args, i->lv_name, shared_from_this());
              res->dump();
            } else {
              llvm::Function* fun = module->getFunction(i->fname);
              if (fun == nullptr) {
                throw std::runtime_error("function could not be referenced");
              }
              res = builder->CreateCall(fun, args, i->lv_name);
            }
            namemap.emplace(i->lv_name, res);
          },
          [&, this](const std::shared_ptr<ReturnInst>& i) {
            builder->CreateRet(namemap[i->val]);
          }},
      inst);
}

void LLVMGenerator::doJit() {}

void LLVMGenerator::outputToStream(llvm::raw_ostream& stream) {
  module->print(stream, nullptr, false, true);
}

}  // namespace mimium