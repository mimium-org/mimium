#include "compiler/llvmgenerator.hpp"

namespace mimium {

LLVMGenerator::LLVMGenerator(llvm::LLVMContext& ctx)
    : isjit(true),
      taskfn_typeid(0),
      ctx(ctx),
      mainentry(nullptr),
      currentblock(nullptr),
      module(std::make_unique<llvm::Module>("no_file_name.mmm", ctx)),
      builder(std::make_unique<llvm::IRBuilder<>>(ctx)),
      typeconverter(*builder, *module) {}
void LLVMGenerator::init(std::string filename) {
  module->setSourceFileName(filename);
  module->setModuleIdentifier(filename);
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
llvm::Value* LLVMGenerator::tryfindValue(std::string name) {
  auto map = variable_map[curfunc];
  auto res = map->find(name);

  return (res == map->end()) ? nullptr : res->second;
}
llvm::Value* LLVMGenerator::findValue(std::string name) {

    auto* res= tryfindValue(name);
    if(res==nullptr){
    throw std::runtime_error("variable " + name +
                             " cannot be found in llvm conversion");
  }
  return res;
}

void LLVMGenerator::setValuetoMap(std::string name, llvm::Value* val) {
  auto map = variable_map[curfunc];
  map->try_emplace(name, val);
}

void LLVMGenerator::initJit() {}

LLVMGenerator::~LLVMGenerator() { dropAllReferences(); }
void LLVMGenerator::dropAllReferences() {
  variable_map.clear();
  if (module != nullptr) {
    module->dropAllReferences();
  }
}

auto LLVMGenerator::getType(types::Value& type) -> llvm::Type* {
  return std::visit(typeconverter, type);
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
  setValuetoMap("malloc", res);
}

// Create mimium_main() function it returns address of closure object for dsp()
// function if it exists.

void LLVMGenerator::createMainFun() {
  auto* fntype = llvm::FunctionType::get(builder->getInt8PtrTy(), false);
  auto* mainfun = llvm::Function::Create(
      fntype, llvm::Function::ExternalLinkage, "mimium_main", module.get());
  mainfun->setCallingConv(llvm::CallingConv::C);
  curfunc = mainfun;
  variable_map.emplace(curfunc, std::make_shared<namemaptype>());
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
  setValuetoMap(name, addtask.getCallee());
}

// llvm::Type* LLVMGenerator::getOrCreateTimeStruct(types::Time& t) {
//   llvm::StringRef name = types::toString(t);
//   llvm::Type* res = module->getTypeByName(name);
//   if (res == nullptr) {
//     llvm::Type* containtype = std::visit(
//         overloaded{[&](types::Float& /*f*/) { return builder->getDoubleTy();
//         },
//                    [&](auto& /*v*/) { return builder->getVoidTy(); }},
//         t.val);

//     res = llvm::StructType::create(ctx, {builder->getDoubleTy(),
//     containtype},
//                                    name);
//   }
//   return res;
// }
void LLVMGenerator::preprocess() {
  createMainFun();
  createMiscDeclarations();
  createTaskRegister(true);   // for non-closure function
  createTaskRegister(false);  // for closure
  setBB(mainentry);
}

void LLVMGenerator::generateCode(std::shared_ptr<MIRblock> mir) {
  preprocess();
  for (auto& inst : mir->instructions) {
    visitInstructions(inst, true);
  }
  if (mainentry->getTerminator() == nullptr) {  // insert return
    auto dspaddress = variable_map[curfunc]->find("dsp_cap");
    if (dspaddress != variable_map[curfunc]->end()) {
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
  if (type->isFunctionTy()) {
    t = llvm::ArrayType::get(llvm::PointerType::get(type, 0), 1);
  }
  if (isglobal) {
    auto rawname = "ptr_" + name.str() + "_raw";
    auto size = module->getDataLayout().getTypeAllocSize(t);
    auto sizeinst = llvm::ConstantInt::get(ctx, llvm::APInt(64, size, false));
    auto rawres =
        builder->CreateCall(module->getFunction("malloc"), {sizeinst}, rawname);
    res = builder->CreatePointerCast(rawres, llvm::PointerType::get(t, 0),
                                     "ptr_" + name);
    setValuetoMap(rawname, rawres);
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
  auto& map = variable_map[curfunc];
  auto it = map->find(varname);
  auto it2 = map->find(ptrname);
  if (it != map->cend() && it2 != map->cend()) {
    builder->CreateStore(val_to_store, findValue(ptrname));
    res = true;
  }
  return res;
}
void LLVMGenerator::createAddTaskFn(FcallInst& i, const bool isclosure,
                                    const bool isglobal) {
  auto tv = findValue("ptr_" + i.args[0]);

  auto timeptr = builder->CreateStructGEP(tv, 0);
  auto timeval = builder->CreateLoad(timeptr);
  auto valptr = builder->CreateStructGEP(tv, 1);
  auto val = builder->CreateLoad(valptr);

  auto targetfn = module->getFunction(i.fname);
  auto ptrtofn =
      llvm::ConstantExpr::getBitCast(targetfn, builder->getInt8PtrTy());
  // auto taskid = taskfn_typeid++;
  // tasktype_list.emplace_back(i.type);

  std::vector<llvm::Value*> args = {timeval, ptrtofn, val};
  llvm::Value* addtask_fn;
  if (isclosure) {
    llvm::Value* clsptr = (isglobal) ? findValue(i.fname + "_cap")
                                     : findValue("clsarg_" + i.fname);

    auto* upcasted = builder->CreateBitCast(clsptr, builder->getInt8PtrTy());
    setValuetoMap(i.fname + "_cls_i8", upcasted);
    args.push_back(upcasted);
    addtask_fn = variable_map[mainentry->getParent()]->find("addTask_cls")->second;
  } else {
    addtask_fn =variable_map[mainentry->getParent()]->find("addTask")->second;
  }
  auto* res = builder->CreateCall(addtask_fn, args);
  setValuetoMap(i.lv_name, res);
}
void LLVMGenerator::createFcall(FcallInst& i, std::vector<llvm::Value*>& args) {
  llvm::Value* fun;
  switch (i.ftype) {
    case EXTERNAL: {
      auto it = LLVMBuiltin::ftable.find(i.fname);
      BuiltinFnInfo& fninfo = it->second;
      auto* fntype = llvm::cast<llvm::FunctionType>(getType(fninfo.mmmtype));
      auto fn = module->getOrInsertFunction(fninfo.target_fnname, fntype);
      auto f = llvm::cast<llvm::Function>(fn.getCallee());
      f->setCallingConv(llvm::CallingConv::C);
      fun = f;
    } break;
    case DIRECT:
    case CLOSURE:
      fun = module->getFunction(i.fname);
      if (fun == nullptr) {
        throw std::logic_error("function could not be referenced");
      }
      break;
    default:
      break;
  }

  if (std::holds_alternative<types::Void>(i.type)) {
    builder->CreateCall(fun, args);
  } else {
    auto res = builder->CreateCall(fun, args, i.lv_name);
    setValuetoMap(i.lv_name, res);
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
            setValuetoMap(ptrname, ptr);
          },
          [&, this](NumberInst& i) {
            auto ptrkey = variable_map[curfunc]->find("ptr_" + i.lv_name);
            auto* finst =
                llvm::ConstantFP::get(this->ctx, llvm::APFloat(i.val));
            if (ptrkey !=
                variable_map[curfunc]->end()) {  // case of temporary value
              auto ptr = ptrkey->second;
              builder->CreateStore(finst, ptr);
              auto res = builder->CreateLoad(ptr, i.lv_name);
              setValuetoMap(i.lv_name, res);
            } else {
              setValuetoMap(i.lv_name, finst);
            }
          },
          [&, this](TimeInst& i) {
            //  auto resname = "ptr_" + i->lv_name;
            // types::Time timetype =
            //     std::get<recursive_wrapper<types::Time>>(i->type);
            // auto strtype = getOrCreateTimeStruct(timetype.val);
            // typemap.emplace(resname, strtype);
            // auto* ptr = createAllocation(isglobal,strtype, nullptr,
            // i->lv_name); auto* timepos = builder->CreateStructGEP(strtype,
            // ptr, 0); auto* valpos = builder->CreateStructGEP(strtype, ptr,
            // 1);

            // auto* time =
            //     builder->CreateFPToUI(namemap[i->time],
            //     builder->getDoubleTy());
            // builder->CreateStore(time, timepos);

            // builder->CreateStore(namemap.find(i->val)->second, valpos);
            // setValuetoMap(resname, ptr);
            ///
            auto* type = getType(i.type);
            auto* time = findValue(i.time);
            auto* val = findValue(i.val);
            auto* struct_p =
                createAllocation(isglobal, type, nullptr, i.lv_name);
            auto gep0 = builder->CreateStructGEP(type, struct_p, 0);
            auto gep1 = builder->CreateStructGEP(type, struct_p, 1);
            builder->CreateStore(time, gep0);
            builder->CreateStore(val, gep1);
            setValuetoMap("ptr_" + i.lv_name, struct_p);
          },
          [&, this](RefInst& i) {  // TODO
            auto ptrname = "ptr_" + i.lv_name;
            auto ptrptrname = "ptr_" + ptrname;
            auto* ptrtoptr =
                createAllocation(isglobal, getType(i.type), nullptr, ptrname);
            builder->CreateStore(findValue(ptrname), ptrtoptr);
            auto* ptr = builder->CreateLoad(ptrtoptr, ptrptrname);
            setValuetoMap(ptrname, ptr);
            setValuetoMap(ptrptrname, ptrtoptr);
          },
          [&, this](AssignInst& i) {
            if (std::holds_alternative<types::Float>(i.type)) {
              // copy assignment
              auto ptr = findValue("ptr_" + i.lv_name);
              builder->CreateStore(findValue(i.val), ptr);
              // rename old register name
              // auto oldval = namemap.extract(i.lv_name);
              // oldval.key() += "_o";

              auto* newval = builder->CreateLoad(ptr, i.lv_name);
              setValuetoMap(i.lv_name, newval);
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
            setValuetoMap(i.lv_name, retvalue);
          },
          [&, this](FunInst& i) {
            bool hasfv = !i.freevariables.empty();
            auto* ft =
                llvm::cast<llvm::FunctionType>(getType(i.type));  // NOLINT
            llvm::Function* f = llvm::Function::Create(
                ft, llvm::Function::ExternalLinkage, i.lv_name, module.get());
            auto f_it = f->args().begin();
            curfunc = f;
            variable_map.emplace(f, std::make_shared<namemaptype>());
            std::for_each(i.args.begin(), i.args.end(),
                          [&](std::string s) { (f_it++)->setName(s); });
            // if function is closure,append closure argument, dsp function is
            // forced to be closure function
            if (hasfv || i.lv_name == "dsp") {
              auto it = f->args().end();
              auto lastelem = (--it);
              auto name = "clsarg_" + i.lv_name;
              lastelem->setName(name);
              setValuetoMap(name, (llvm::Value*)lastelem);
            }
            setValuetoMap(i.lv_name, f);
            auto* bb = llvm::BasicBlock::Create(ctx, "entry", f);
            builder->SetInsertPoint(bb);
            currentblock = bb;
            f_it = f->args().begin();
            std::for_each(i.args.begin(), i.args.end(),
                          [&](std::string s) { setValuetoMap(s, f_it++); });
            if (hasfv) {
              auto arg_end = f->arg_end();
              llvm::Value* lastarg = --arg_end;
              for (int id = 0; id < i.freevariables.size(); id++) {
                std::string newname = i.freevariables[id];
                llvm::Value* gep = builder->CreateStructGEP(lastarg, id, "fv");
                auto ptrname = "ptr_" + newname;
                llvm::Value* ptrload = builder->CreateLoad(gep, ptrname);
                setValuetoMap(ptrname, ptrload);
                auto* ptype = llvm::cast<llvm::PointerType>(ptrload->getType());
                if (ptype->getElementType()->isFirstClassType()) {
                  llvm::Value* valload = builder->CreateLoad(ptrload, newname);
                  setValuetoMap(newname, valload);
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
            curfunc = mainentry->getParent();
          },
          [&, this](MakeClosureInst& i) {
            // store the capture address to memory
            auto* structty = getType(i.capturetype);
            auto capptrname = i.fname + "_cap";
            llvm::Value* capture_ptr =
                createAllocation(isglobal, structty, nullptr, capptrname);

            unsigned int idx = 0;
            for (auto& cap : i.captures) {
              auto ptrname = "ptr_" + cap;
              llvm::Value* fvptr =  tryfindValue(ptrname);
              if (fvptr == nullptr) {
                auto v = findValue(cap);
                // heap allocation! possibly memory leak
                fvptr = createAllocation(true, v->getType(), nullptr, cap);
                setValuetoMap(ptrname, fvptr);
                builder->CreateStore(v, fvptr);
              }
              auto pp = builder->CreateStructGEP(capture_ptr, idx);
              builder->CreateStore(findValue(ptrname), pp);
              idx += 1;
            }
            setValuetoMap(capptrname, capture_ptr);

            // auto f = module->getFunction(i.fname);

            // auto* atype =
            // llvm::StructType::create({f->getType()},"container_"+i.fname);
            // auto* gptr = (llvm::GlobalVariable*)module->getOrInsertGlobal(
            //     "ptr_to_" + i.fname, atype);
            // gptr->setInitializer(llvm::ConstantStruct::get(atype, {f}));
            // gptr->setConstant(true);
            // gptr->setLinkage(llvm::GlobalValue::LinkageTypes::ExternalLinkage);
            // auto ptrtoptr = builder->CreateInBoundsGEP(
            //     atype, gptr, llvm::ConstantInt::get(ctx, llvm::APInt(32, 0)),
            //     "ptrptr_" + i.fname);
            // auto ptr = builder->CreateLoad(ptrtoptr, "ptr_" + i.fname);
            // // store the pointer to function(1 size array type)
            // // auto* ptrtofntype = getType(i.type);
            // // llvm::Value* fn_ptr =
            // // createAllocation(isglobal,ptrtofntype,nullptr,i.fname+"_cls");
            // // llvm::Value* f = module->getFunction(i.fname);
            // // auto res = builder->CreateInsertValue(fn_ptr, f, 0);
            // namemap.emplace("ptr_" + i.fname, ptr);
          },
          [&, this](FcallInst& i) {
            bool isclosure = i.ftype == CLOSURE;
            std::vector<llvm::Value*> args;
            auto m = variable_map[curfunc];
            for (auto& a : i.args) {
              // TODO(tomoya): need to add type infomation to argument...
              auto it = m->find(a);
              if (it != m->end()) {
                args.emplace_back(it->second);
              } else {
                args.emplace_back(findValue("ptr_" + a));
              }
            }

            if (isclosure) {
              auto m = variable_map[curfunc];
              auto it = m->find(i.fname + "_cap");
              if(it == m->end()){
                dumpvars();
              args.emplace_back(findValue("clsarg_" + i.fname));
              }else{
                args.emplace_back(it->second);
              }
            }
            if (i.istimed) {  // if arguments is timed value, call addTask
              createAddTaskFn(i, isclosure, isglobal);
            } else {
              createFcall(i, args);
            }
          },
          [&, this](ReturnInst& i) { 
            auto v= tryfindValue(i.val);
            if(v==nullptr){
              //case of returning function;
              v= module->getFunction(i.val);
            }
            builder->CreateRet(v);
             }},
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