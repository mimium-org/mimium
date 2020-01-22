#include "compiler/llvmgenerator.hpp"

namespace mimium {

LLVMGenerator::LLVMGenerator(llvm::LLVMContext& ctx)
    : isjit(true),
      taskfn_typeid(0),
      tasktype_list(),
      ctx(ctx),
      mainentry(nullptr),
      currentblock(nullptr) {
  builder = std::make_unique<llvm::IRBuilder<>>(ctx);
}
void LLVMGenerator::init(std::string filename) {
  module = std::make_unique<llvm::Module>(filename, ctx);
  // module->setDataLayout(LLVMGetDefaultTargetTriple());
}
void LLVMGenerator::setDataLayout() {
  module->setDataLayout(llvm::sys::getProcessTriple());
}
void LLVMGenerator::setDataLayout(const llvm::DataLayout& dl) {
  module->setDataLayout(dl);
}

void LLVMGenerator::reset(std::string filename) {
  dropAllReferences();
  init(filename);
}
llvm::Value* LLVMGenerator::findValue(std::string name) {
  llvm::Value* res;
  auto rawval = namemap.find(name);
  if (rawval == namemap.end()) {
    auto fvname = namemap.find("fv_" + name);
    res = (fvname == namemap.end()) ? nullptr : fvname->second;
  } else {
    res = rawval->second;
  }

  return res;
}

void LLVMGenerator::initJit() {}

LLVMGenerator::~LLVMGenerator() { dropAllReferences(); }
void LLVMGenerator::dropAllReferences() {
  namemap.clear();
  if (module != nullptr) {
    module->dropAllReferences();
  }
}

auto LLVMGenerator::getType(types::Value& type) -> llvm::Type* {
  return std::visit(TypeConverter(*builder, *module), type);
}

llvm::Function* LLVMGenerator::getForeignFunction(std::string name) {
  auto& [type, targetname] = LLVMBuiltin::ftable.find(name)->second;
  auto fnc = module->getOrInsertFunction(
      name, llvm::cast<llvm::FunctionType>(getType(type)));
  auto* fn = llvm::cast<llvm::Function>(fnc.getCallee());
  fn->setCallingConv(llvm::CallingConv::C);
  return fn;
}

void LLVMGenerator::setBB(llvm::BasicBlock* newblock) {
  builder->SetInsertPoint(newblock);
}
void LLVMGenerator::createMiscDeclarations() {
  // create malloc
  auto* malloctype = llvm::FunctionType::get(builder->getInt8PtrTy(),
                                             {builder->getInt64Ty()}, false);
  auto res = module->getOrInsertFunction("malloc", malloctype).getCallee();
  namemap.emplace("malloc", res);
}

// Create mimium_main() function it returns address of closure object for dsp()
// function if it exists.

void LLVMGenerator::createMainFun() {
  auto* fntype = llvm::FunctionType::get(builder->getInt8PtrTy(), false);
  auto* mainfun = llvm::Function::Create(
      fntype, llvm::Function::ExternalLinkage, "mimium_main", module.get());
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
void LLVMGenerator::createTaskRegister(bool isclosure = false) {
  std::vector<llvm::Type*> argtypes = {
      builder->getDoubleTy(),   // time
      builder->getInt8PtrTy(),  // address to function
      builder->getDoubleTy()    // argument(single)
  };
  std::string name = "addTask";
  if (isclosure) {
    argtypes.push_back(builder->getInt8PtrTy());
    name = "addTask_cls";
  }  // address to closure args(instead of void* type)
  auto* fntype = llvm::FunctionType::get(builder->getVoidTy(), argtypes, false);
  auto addtask = module->getOrInsertFunction(name, fntype);
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
  typemap.emplace(name, fntype);
  namemap.emplace(name, addtask.getCallee());
}

llvm::Type* LLVMGenerator::getOrCreateTimeStruct(types::Time& t) {
  llvm::StringRef name = types::toString(t);
  llvm::Type* res = module->getTypeByName(name);
  if (res == nullptr) {
    llvm::Type* containtype = std::visit(
        overloaded{[&](types::Float& /*f*/) { return builder->getDoubleTy(); },
                   [&](auto& /*v*/) { return builder->getVoidTy(); }},
        t.val);

    res = llvm::StructType::create(ctx, {builder->getDoubleTy(), containtype},
                                   name);
  }
  return res;
}
void LLVMGenerator::preprocess() {
  createMiscDeclarations();
  createTaskRegister(true);   // for non-closure function
  createTaskRegister(false);  // for closure
  createMainFun();
  setBB(mainentry);
}

void LLVMGenerator::generateCode(std::shared_ptr<MIRblock> mir) {
  preprocess();
  for (auto& inst : mir->instructions) {
    visitInstructions(inst, true);
  }
  if (mainentry->getTerminator() == nullptr) {  // insert return
    auto dspaddress = namemap.find("dsp_cap");
    if (dspaddress != namemap.end()) {
      builder->CreateRet(dspaddress->second);
    } else {
      builder->CreateRet(
          llvm::ConstantPointerNull::get(builder->getInt8PtrTy()));
    }
  }
}

// Creates Allocation instruction or call malloc function depends on context
llvm::Value* LLVMGenerator::createAllocation(bool isglobal, llvm::Type* type,
                                             llvm::Value* ArraySize = nullptr,
                                             const llvm::Twine& name = "") {
  llvm::Value* res = nullptr;
  llvm::Type* t = type;
  if(type->isFunctionTy()){
      t = llvm::ArrayType::get(llvm::PointerType::get(type,0),1);
  }
  if (isglobal) {
    auto rawname = "ptr_" + name.str() + "_raw";
    auto size = module->getDataLayout().getTypeAllocSize(t);
    auto sizeinst = llvm::ConstantInt::get(ctx, llvm::APInt(64, size, false));
    auto rawres =
        builder->CreateCall(module->getFunction("malloc"), {sizeinst}, rawname);
    res = builder->CreatePointerCast(rawres, llvm::PointerType::get(t, 0),
                                     "ptr_" + name);
    namemap.try_emplace(rawname, rawres);
  } else {
    res = builder->CreateAlloca(type, ArraySize, "ptr_" + name);
  }
  return res;
};
// Create StoreInst if storing to already allocated value
bool LLVMGenerator::createStoreOw(std::string varname,
                                  llvm::Value* val_to_store) {
  bool res = false;
  auto ptrname = "ptr_" + varname;
  auto it = namemap.find(varname);
  auto it2 = namemap.find(ptrname);
  if (it != namemap.cend() && it2 != namemap.cend()) {
    builder->CreateStore(val_to_store, findValue(ptrname));
    res = true;
  }
  return res;
}
void LLVMGenerator::createAddTaskFn(FcallInst& i, const bool isclosure,
                                    const bool isglobal) {
  auto tv = findValue(i.args[0]);
  auto timeval = builder->CreateExtractValue(tv, 0);
  auto val = builder->CreateExtractValue(tv, 1);
  auto targetfn = llvm::cast<llvm::Function>(findValue(i.fname));
  auto ptrtofn =
      llvm::ConstantExpr::getBitCast(targetfn, builder->getInt8PtrTy());
  // auto taskid = taskfn_typeid++;
  tasktype_list.emplace_back(i.type);

  std::vector<llvm::Value*> args = {timeval, ptrtofn, val};
  llvm::Value* addtask_fn;
  if (isclosure) {
    llvm::Value* clsptr = (isglobal) ? findValue("ptr_" +i.fname + "_cap")
                                     : findValue("clsarg_" + i.fname);

    auto* upcasted = builder->CreateBitCast(clsptr, builder->getInt8PtrTy());
    namemap.emplace(i.fname + "_cls_i8", upcasted);
    args.push_back(upcasted);
    addtask_fn = findValue("addTask_cls");
  } else {
    addtask_fn = findValue("addTask");
  }
  auto* res = builder->CreateCall(addtask_fn, args);
  namemap.emplace(i.lv_name, res);
}
void LLVMGenerator::createFcall(FcallInst& i, std::vector<llvm::Value*>& args) {
  llvm::Value* fun;
  switch (i.ftype) {
    case EXTERNAL:
    {
    auto it = LLVMBuiltin::ftable.find(i.fname);
    BuiltinFnInfo& fninfo = it->second;
    auto* fntype = llvm::cast<llvm::FunctionType>(getType(fninfo.mmmtype));
    auto fn = module->getOrInsertFunction(fninfo.target_fnname, fntype);
    auto f = llvm::cast<llvm::Function>(fn.getCallee());
    f->setCallingConv(llvm::CallingConv::C);
    fun = f;
    }
    break;
    case DIRECT:
    fun = module->getFunction(i.fname);
    if (fun == nullptr) {
      throw std::logic_error("function could not be referenced");
    }
  break;
  case CLOSURE:{
    auto* arrptr = findValue("ptr_"+i.fname);
    auto* v = builder->CreateExtractValue(arrptr, 0);
    fun=v;

  }
  break;
  default:
  break;
  }

  if (std::holds_alternative<types::Void>(i.type)) {
    fun->dump();
    for(auto&a:args) a->dump();
    builder->CreateCall(fun, args);
  } else {
    auto res = builder->CreateCall(fun, args, i.lv_name);
    namemap.emplace(i.lv_name, res);
  }
}
void LLVMGenerator::visitInstructions(Instructions& inst, bool isglobal) {
  std::visit(
      overloaded{
          [](auto i) {},
          [&, this](AllocaInst& i) {
            auto ptrname = "ptr_" + i.lv_name;
            auto* type = getType(i.type);
            auto* ptr = createAllocation(isglobal, type, nullptr, i.lv_name);
            typemap.emplace(ptrname, type);
            namemap.emplace(ptrname, ptr);
          },
          [&, this](NumberInst& i) {
            auto ptr = findValue("ptr_" + i.lv_name);
            auto* finst =
                llvm::ConstantFP::get(this->ctx, llvm::APFloat(i.val));
            if (ptr != nullptr) {  // case of temporary value
              builder->CreateStore(finst, ptr);
              auto res = builder->CreateLoad(ptr, i.lv_name);
              namemap.emplace(i.lv_name, res);
            } else {
              namemap.emplace(i.lv_name, finst);
            }
          },
          [&, this](TimeInst& i) {
            auto* type = typemap[i.lv_name];
            if (type == nullptr) {
              type = getType(i.type);
              typemap.try_emplace(i.lv_name, type);
            }
            auto* time = builder->CreateFPToUI(findValue(i.time),
                                               builder->getDoubleTy());
            auto* val = findValue(i.val);
            auto* fpzero = llvm::ConstantFP::get(builder->getDoubleTy(), 0.);
            auto* struct_p = llvm::ConstantStruct::get(
                llvm::cast<llvm::StructType>(type), {fpzero, fpzero});
            auto tmp1 = builder->CreateInsertValue(struct_p, time, 0);
            auto tmp2 = builder->CreateInsertValue(tmp1, val, 1);
            namemap.emplace(i.lv_name, tmp2);
          },
          [&, this](RefInst& i) {  // TODO
            auto ptrname = "ptr_" + i.lv_name;
            auto ptrptrname = "ptr_" + ptrname;
            auto* ptrtoptr = createAllocation(
                isglobal, llvm::PointerType::get(typemap[i.val], 0), nullptr,
                ptrname);
            builder->CreateStore(findValue(ptrname), ptrtoptr);
            auto* ptr = builder->CreateLoad(ptrtoptr, ptrptrname);
            namemap.emplace(ptrname, ptr);
            namemap.emplace(ptrptrname, ptrtoptr);
          },
          [&, this](AssignInst& i) {
            if (std::holds_alternative<types::Float>(i.type)) {
              // copy assignment
              auto ptr = findValue("ptr_" + i.lv_name);
              if (ptr == nullptr) {
                // try argument ,,,stupid
                ptr = findValue("ptr_fv_" + i.lv_name);
              }
              builder->CreateStore(findValue(i.val), ptr);
              // rename old register name
              // auto oldval = namemap.extract(i.lv_name);
              // oldval.key() += "_o";

              auto* newval = builder->CreateLoad(ptr, i.lv_name);
              namemap.emplace(i.lv_name, newval);
            }
          },
          [&, this](OpInst& i) {
            llvm::Value* retvalue;
            auto* lhs = findValue(i.lhs);
            auto* rhs = findValue(i.rhs);
            switch (i.getOPid()) {
              case OP_ID::ADD:
                retvalue = builder->CreateFAdd(lhs, rhs, i.lv_name);
                break;
              case OP_ID::SUB:
                retvalue = builder->CreateFSub(lhs, rhs, i.lv_name);
                break;
              case OP_ID::MUL:
                retvalue = builder->CreateFMul(lhs, rhs, i.lv_name);
                break;
              case OP_ID::DIV:
                retvalue = builder->CreateFDiv(lhs, rhs, i.lv_name);
                break;
              case OP_ID::MOD:
                retvalue = builder->CreateCall(getForeignFunction("fmod"),
                                               {lhs, rhs}, i.lv_name);
                break;
              default:
                retvalue = builder->CreateUnreachable();
                break;
            }
            namemap.emplace(i.lv_name, retvalue);
          },
          [&, this](FunInst& i) {
            bool hasfv = !i.freevariables.empty();
            auto* ft =
                llvm::cast<llvm::FunctionType>(getType(i.type));  // NOLINT
            llvm::Function* f = llvm::Function::Create(
                ft, llvm::Function::ExternalLinkage, i.lv_name, module.get());
            auto f_it = f->args().begin();

            std::for_each(i.args.begin(), i.args.end(),
                          [&](std::string s) { (f_it++)->setName(s); });
            // if function is closure,append closure argument, dsp function is
            // forced to be closure function
            if (hasfv || i.lv_name == "dsp") {
              auto it = f->args().end();
              auto lastelem = (--it);
              auto name = "clsarg_" + i.lv_name;
              lastelem->setName(name);
              namemap.emplace(name, (llvm::Value*)lastelem);
            }
            namemap.emplace(i.lv_name, f);
            auto* bb = llvm::BasicBlock::Create(ctx, "entry", f);
            builder->SetInsertPoint(bb);
            currentblock = bb;
            f_it = f->args().begin();
            std::for_each(i.args.begin(), i.args.end(),
                          [&](std::string s) { namemap.emplace(s, f_it++); });
            if(hasfv){
            auto arg_end = f->arg_end();
            llvm::Value* lastarg = --arg_end;
            llvm::Value* lastarg_str = builder->CreateLoad(lastarg);
            for (int id = 0; id < i.freevariables.size(); id++) {
              std::string newname = "fv_" + i.freevariables[id];
              // llvm::Value* gep =
              // builder->CreateStructGEP(lastarg_str->getType(), lastarg_str,
              // id, "fv");
              auto ptrname = "ptr_" + newname;
              llvm::Value* ptrload = builder->CreateExtractValue(
                  lastarg_str, id, "ptr_" + newname);
              namemap.try_emplace(ptrname, ptrload);
              auto* ptype = llvm::cast<llvm::PointerType>(ptrload->getType());
              if (ptype->getElementType()->isFirstClassType()) {
                llvm::Value* valload = builder->CreateLoad(ptrload, newname);
                namemap.try_emplace(newname, valload);
              }
            }
            }
            for (auto& cinsts : i.body->instructions) {
              visitInstructions(cinsts, false);
            }
            if (currentblock->getTerminator() == nullptr &&
                ft->getReturnType()->isVoidTy()) {
              builder->CreateRetVoid();
            }
            setBB(mainentry);
            currentblock = mainentry;
          },
          [&, this](MakeClosureInst& i) {
            // store the capture address to memory
            auto* structptrtype = (llvm::PointerType*)getType(i.capturetype);
            auto* structty = structptrtype->getElementType();
            auto capptrname= "ptr_" + i.fname + "_cap";
            llvm::Value* capture_ptr = createAllocation(
                isglobal, structty, nullptr, capptrname);
            auto* capturetuple =
                builder->CreateLoad(capture_ptr, i.fname + "_cap");
            unsigned int idx = 0;
            for (auto& cap : i.captures) {
              llvm::Value* fvptr = findValue("ptr_" + cap);
              if (fvptr == nullptr) {
                auto v = findValue(cap);
                // heap allocation! possibly memory leak
                fvptr = createAllocation(true, v->getType(), nullptr, cap);
                builder->CreateStore(v, fvptr);
              }
              llvm::Value* insert =
                  builder->CreateInsertValue(capturetuple, fvptr, {idx}, "");
              idx += 1;
            }
            namemap.try_emplace(capptrname,capture_ptr);

            auto f = module->getFunction(i.fname);
            auto* atype = llvm::ArrayType::get(f->getType(), 1);
            auto* gptr = (llvm::GlobalVariable*)module->getOrInsertGlobal(
                "ptr_to_" + i.fname, atype);
            gptr->setInitializer(llvm::ConstantArray::get(atype, {f}));
            gptr->setConstant(true);
            gptr->setLinkage(llvm::GlobalValue::LinkageTypes::PrivateLinkage);
            auto ptrtoptr = builder->CreateInBoundsGEP(
                atype, gptr, llvm::ConstantInt::get(ctx, llvm::APInt(32, 0)),
                "ptrptr_" + i.fname);
            auto ptr = builder->CreateLoad(ptrtoptr, "ptr_" + i.fname);
            // store the pointer to function(1 size array type)
            // auto* ptrtofntype = getType(i.type);
            // llvm::Value* fn_ptr =
            // createAllocation(isglobal,ptrtofntype,nullptr,i.fname+"_cls");
            // llvm::Value* f = module->getFunction(i.fname);
            // auto res = builder->CreateInsertValue(fn_ptr, f, 0);
            namemap.emplace("ptr_" + i.fname, ptr);
          },
          [&, this](FcallInst& i) {
            bool isclosure = i.ftype == CLOSURE;
            std::vector<llvm::Value*> args;
            std::for_each(i.args.begin(), i.args.end(),
                          [&](auto& a) { args.emplace_back(findValue(a)); });
            if (isclosure) {
              args.emplace_back(findValue("ptr_"+i.fname + "_cap"));
            }
            if (i.istimed) {  // if arguments is timed value, call addTask
              createAddTaskFn(i, isclosure, isglobal);
            } else {
              createFcall(i, args);
            }
          },
          [&, this](ReturnInst& i) { builder->CreateRet(findValue(i.val)); }},
      inst);
}

llvm::Error LLVMGenerator::doJit(const size_t opt_level) {
  return jitengine->addModule(
      std::move(module));  // do JIT compilation for module
}
void* LLVMGenerator::execute() {
  llvm::Error err = doJit();
  Logger::debug_log(err, Logger::ERROR);
  auto mainfun = jitengine->lookup("mimium_main");
  Logger::debug_log(mainfun, Logger::ERROR);
  auto fnptr =
      llvm::jitTargetAddressToPointer<void* (*)()>(mainfun->getAddress());
  //
  void* res = fnptr();
  return res;
}
void LLVMGenerator::outputToStream(llvm::raw_ostream& stream) {
  module->print(stream, nullptr, false, true);
}

}  // namespace mimium