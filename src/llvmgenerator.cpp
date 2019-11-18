#include "llvmgenerator.hpp"
namespace mimium {
LLVMGenerator::LLVMGenerator(std::string _filename) {
  builder = std::make_unique<llvm::IRBuilder<>>(ctx);
  module = std::make_shared<llvm::Module>(_filename, ctx);
  ;
}
// LLVMGenerator::LLVMGenerator(llvm::LLVMContext& _ctx,std::string _filename){
//     // ctx.reset();
//     // ctx = std::move(&_ctx);
// }

LLVMGenerator::~LLVMGenerator() {

}

std::shared_ptr<llvm::Module> LLVMGenerator::getModule() {
  if (module) {
    return module;
  } else {
    return std::make_shared<llvm::Module>("null", ctx);
  }
}

llvm::Type* LLVMGenerator::getType(types::Value type) {
  return std::visit(
      overloaded{
          [this](types::Float f) { return llvm::Type::getDoubleTy(ctx); },
          [this](recursive_wrapper<types::Function> rf) {
            auto f  = (types::Function)rf;
            std::vector<llvm::Type*> args;
            auto* rettype = getType(f.getReturnType());
            for (auto& a : f.getArgTypes()) {
              auto* atype = getType(a);
              args.push_back(atype);
            }
            // upcast
            llvm::Type* res = llvm::FunctionType::get(
                rettype, args, false);  // what is final parameter isVarArg??
            return res;
          },
          [this](auto t) {
            throw std::runtime_error("not a function");
            return llvm::Type::getDoubleTy(ctx);
            ;
          }},
      type);
}

void LLVMGenerator::preprocess() {
  auto* fntype = llvm::FunctionType::get(llvm::Type::getInt64Ty(ctx), false);
  auto* mainfun = llvm::Function::Create(
      fntype, llvm::Function::ExternalLinkage, "main", module.get());
  std::unique_ptr<llvm::BasicBlock> ptr(llvm::BasicBlock::Create(ctx, "entry", mainfun) );
  mainentry = std::move(ptr);
  builder->SetInsertPoint(mainentry.get());
}

void LLVMGenerator::generateCode(std::shared_ptr<MIRblock> mir) {
  preprocess();
  for (auto& inst : mir->instructions) {
    visitInstructions(inst);
  }
    // mainentry.reset();
}

void LLVMGenerator::visitInstructions(Instructions& inst) {
  std::visit(overloaded {
    [](auto i) {},
        [&, this](std::shared_ptr<NumberInst> i) {
          auto ptr = builder->CreateAlloca(llvm::Type::getDoubleTy(ctx),
                                           nullptr);
          auto finst = llvm::ConstantFP::get(this->ctx, llvm::APFloat(i->val));
          builder->CreateStore(finst, ptr);
          auto* load = builder->CreateLoad(ptr,i->lv_name);
          namemap.emplace(i->lv_name, load);

        },
        [&, this](std::shared_ptr<OpInst> i) {
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
        [&, this](std::shared_ptr<FunInst> i) {
          auto* ft = static_cast<llvm::FunctionType*>(getType(i->type));
          llvm::Function* f = llvm::Function::Create(
              ft, llvm::Function::ExternalLinkage, i->lv_name, module.get());
          int idx = 0;
          for (auto& arg : f->args()) {
            arg.setName(i->args[idx++]);
          }
          namemap.emplace(i->lv_name, f);
          auto* bb = llvm::BasicBlock::Create(ctx,"entry",f);
          builder->SetInsertPoint(bb);
          idx=0;
          for(auto& arg : f->args()){
              namemap.emplace(i->args[idx++],&arg);
          }
          for(auto& cinsts : i->body->instructions){
              visitInstructions(cinsts);
          }
          builder->SetInsertPoint(mainentry.get());
        },
        [&,this](std::shared_ptr<FcallInst> i){
            llvm::Function* fun =  module->getFunction(i->fname);
            if(!fun) throw std::runtime_error("function could not be referenced");
            std::vector<llvm::Value*> args;
            for(auto& a : i->args){
                args.push_back(namemap[a]);
            }
            auto* ptr = builder->CreateCall(fun,args,i->lv_name);
            namemap.emplace(i->lv_name,ptr);
        },
        [&,this](std::shared_ptr<ReturnInst> i){
              builder->CreateRet(namemap[i->val]);
        }
  } ,inst);
}

void LLVMGenerator::outputToStream(llvm::raw_ostream& stream) {
  module->print(stream, nullptr, false, true);
}

}  // namespace mimium