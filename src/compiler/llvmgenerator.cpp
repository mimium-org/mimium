#include "compiler/llvmgenerator.hpp"

namespace mimium {

LLVMGenerator::LLVMGenerator(llvm::LLVMContext& ctx, TypeEnv& typeenv)
    : isjit(true),
      taskfn_typeid(0),
      ctx(ctx),
      mainentry(nullptr),
      currentblock(nullptr),
      module(std::make_unique<llvm::Module>("no_file_name.mmm", ctx)),
      builder(std::make_unique<llvm::IRBuilder<>>(ctx)),
      typeenv(typeenv),
      codegenvisitor(*this),
      typeconverter(*builder, *module) {}
void LLVMGenerator::init(std::string filename) {
  module->setSourceFileName(filename);
  module->setModuleIdentifier(filename);
  module->setTargetTriple(LLVMGetDefaultTargetTriple());
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

void LLVMGenerator::initJit() {}

LLVMGenerator::~LLVMGenerator() { dropAllReferences(); }
void LLVMGenerator::dropAllReferences() {
  variable_map.clear();
  if (module != nullptr) {
    module->dropAllReferences();
  }
}

llvm::Type* LLVMGenerator::getType(types::Value& type) {
  return std::visit(typeconverter, type);
}

llvm::Type* LLVMGenerator::getType(const std::string& name) {
  return getType(typeenv.find(name));
}
llvm::Type* LLVMGenerator::getClosureToFunType(types::Value& type) {
  auto aliasty = std::get<recursive_wrapper<types::Alias>>(type).getraw();
  auto clsty =
      std::get<recursive_wrapper<types::Closure>>(aliasty.target).getraw();

  auto fty =
      std::get<recursive_wrapper<types::Function>>(clsty.fun.val).getraw();
  fty.arg_types.emplace_back(types::Ref(clsty.captures));
  types::Value v = fty;
  return std::visit(typeconverter, v);
}
void LLVMGenerator::switchToMainFun() {
  setBB(mainentry);
  currentblock = mainentry;
  curfunc = mainentry->getParent();
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
    auto dspaddress = variable_map[curfunc]->find("ptr_dsp_cls_raw");
    if (dspaddress != variable_map[curfunc]->end()) {
      builder->CreateRet(dspaddress->second);
    } else {
      builder->CreateRet(
          llvm::ConstantPointerNull::get(builder->getInt8PtrTy()));
    }
  }
}

// Creates Allocation instruction or call malloc function depends on context
llvm::Value* LLVMGenerator::CodeGenVisitor::createAllocation(
    bool isglobal, llvm::Type* type, llvm::Value* ArraySize = nullptr,
    const llvm::Twine& name = "") {
  llvm::Value* res = nullptr;
  llvm::Type* t = type;
  if (type->isFunctionTy()) {
    t = llvm::ArrayType::get(llvm::PointerType::get(type, 0), 1);
  }
  if (isglobal) {
    auto rawname = "ptr_" + name.str() + "_raw";
    auto size = G.module->getDataLayout().getTypeAllocSize(t);
    auto sizeinst = llvm::ConstantInt::get(G.ctx, llvm::APInt(64, size, false));
    auto rawres = G.builder->CreateCall(G.module->getFunction("malloc"),
                                        {sizeinst}, rawname);
    res = G.builder->CreatePointerCast(rawres, llvm::PointerType::get(t, 0),
                                       "ptr_" + name);
    G.setValuetoMap(rawname, rawres);
  } else {
    res = G.builder->CreateAlloca(type, ArraySize, "ptr_" + name);
  }
  return res;
};
// Create StoreInst if storing to already allocated value
bool LLVMGenerator::CodeGenVisitor::createStoreOw(std::string varname,
                                                  llvm::Value* val_to_store) {
  bool res = false;
  auto ptrname = "ptr_" + varname;
  auto& map = G.variable_map[G.curfunc];
  auto it = map->find(varname);
  auto it2 = map->find(ptrname);
  if (it != map->cend() && it2 != map->cend()) {
    G.builder->CreateStore(val_to_store, G.findValue(ptrname));
    res = true;
  }
  return res;
}

void LLVMGenerator::visitInstructions(Instructions& inst, bool isglobal) {
  codegenvisitor.isglobal = isglobal;
  std::visit(codegenvisitor, inst);
}

void LLVMGenerator::CodeGenVisitor::operator()(NumberInst& i) {
  auto* finst = llvm::ConstantFP::get(G.ctx, llvm::APFloat(i.val));
  auto ptr = G.tryfindValue("ptr_" + i.lv_name);
  if (ptr != nullptr) {  // case of temporary value
    G.builder->CreateStore(finst, ptr);
    auto res = G.builder->CreateLoad(ptr, i.lv_name);
    G.setValuetoMap(i.lv_name, res);
  } else {
    G.setValuetoMap(i.lv_name, finst);
  }
}
void LLVMGenerator::CodeGenVisitor::operator()(AllocaInst& i) {
  auto ptrname = "ptr_" + i.lv_name;
  auto* type = G.getType(i.lv_name);
  auto* ptr = createAllocation(isglobal, type, nullptr, i.lv_name);
  G.setValuetoMap(ptrname, ptr);
}
void LLVMGenerator::CodeGenVisitor::operator()(RefInst& i) {
  auto ptrname = "ptr_" + i.lv_name;
  auto ptrptrname = "ptr_" + ptrname;
  auto* ptrtoptr =
      createAllocation(isglobal, G.getType(i.type), nullptr, ptrname);
  G.builder->CreateStore(G.findValue(ptrname), ptrtoptr);
  auto* ptr = G.builder->CreateLoad(ptrtoptr, ptrptrname);
  G.setValuetoMap(ptrname, ptr);
  G.setValuetoMap(ptrptrname, ptrtoptr);
}
void LLVMGenerator::CodeGenVisitor::operator()(AssignInst& i) {
  if (types::isPrimitive(i.type)) {
    // copy assignment
    auto ptr = G.findValue("ptr_" + i.lv_name);
    G.builder->CreateStore(G.findValue(i.val), ptr);
    auto* newval = G.builder->CreateLoad(ptr, i.lv_name);
    G.setValuetoMap(i.lv_name, newval);
  } else {
  }
}
void LLVMGenerator::CodeGenVisitor::operator()(TimeInst& i) {
  auto* type = G.getType(i.type);
  auto* time = G.findValue(i.time);
  auto* val = G.findValue(i.val);
  auto* struct_p = createAllocation(isglobal, type, nullptr, i.lv_name);
  auto gep0 = G.builder->CreateStructGEP(type, struct_p, 0);
  auto gep1 = G.builder->CreateStructGEP(type, struct_p, 1);
  G.builder->CreateStore(time, gep0);
  G.builder->CreateStore(val, gep1);
  G.setValuetoMap("ptr_" + i.lv_name, struct_p);
}

void LLVMGenerator::CodeGenVisitor::operator()(OpInst& i) {
  llvm::Value* retvalue;
  auto* lhs = G.findValue(i.lhs);
  auto* rhs = G.findValue(i.rhs);
  switch (i.getOPid()) {
    case OP_ID::ADD:
      retvalue = G.builder->CreateFAdd(lhs, rhs, i.lv_name);
      break;
    case OP_ID::SUB:
      retvalue = G.builder->CreateFSub(lhs, rhs, i.lv_name);
      break;
    case OP_ID::MUL:
      retvalue = G.builder->CreateFMul(lhs, rhs, i.lv_name);
      break;
    case OP_ID::DIV:
      retvalue = G.builder->CreateFDiv(lhs, rhs, i.lv_name);
      break;
    case OP_ID::MOD:
      retvalue = G.builder->CreateCall(G.getForeignFunction("fmod"), {lhs, rhs},
                                       i.lv_name);
      break;
    default:
      retvalue = G.builder->CreateUnreachable();
      break;
  }
  G.setValuetoMap(i.lv_name, retvalue);
}
void LLVMGenerator::CodeGenVisitor::operator()(FunInst& i) {
  bool hasfv = !i.freevariables.empty();
  llvm::FunctionType* ft;
  if (hasfv) {
    auto& type = G.typeenv.find(i.lv_name + "_cls");
    auto clsty = llvm::cast<llvm::StructType>(G.getType(type));
    auto funty = llvm::cast<llvm::PointerType>(clsty->getElementType(0))
                     ->getElementType();
    ft = llvm::cast<llvm::FunctionType>(funty);
  } else {
    ft = llvm::cast<llvm::FunctionType>(
        G.getType(G.typeenv.find(i.lv_name)));  // NOLINT
  }

  llvm::Function* f = llvm::Function::Create(
      ft, llvm::Function::ExternalLinkage, i.lv_name, G.module.get());
  G.setValuetoMap(i.lv_name, f);

  G.curfunc = f;
  G.variable_map.emplace(f, std::make_shared<namemaptype>());

  auto f_it = f->args().begin();
  for (auto& a : i.args) {
    (f_it)->setName(a);
    G.setValuetoMap(a, f_it++);
  }
  // if function is closure,append closure argument, dsp function is
  // forced to be closure function
  if (hasfv || i.lv_name == "dsp") {
    auto it = f->args().end();
    auto lastelem = (--it);
    auto name = "clsarg_" + i.lv_name;
    lastelem->setName(name);
    G.setValuetoMap(name, lastelem);
  }
  auto* bb = llvm::BasicBlock::Create(G.ctx, "entry", f);
  G.builder->SetInsertPoint(bb);
  G.currentblock = bb;
  if (hasfv) {
    setFvsToMap(i, f);
  }
  for (auto& cinsts : i.body->instructions) {
    G.visitInstructions(cinsts, false);
  }
  if (G.currentblock->getTerminator() == nullptr &&
      ft->getReturnType()->isVoidTy()) {
    G.builder->CreateRetVoid();
  }
  G.switchToMainFun();
}
void LLVMGenerator::CodeGenVisitor::setFvsToMap(FunInst& i, llvm::Function* f) {
  auto arg_end = f->arg_end();
  auto* lastarg = --arg_end;
  for (int id = 0; id < i.freevariables.size(); id++) {
    std::string newname = i.freevariables[id];
    auto* gep = G.builder->CreateStructGEP(lastarg, id, "fv");
    // auto ptrname = "ptr_" + newname;
    // auto* ptrload = G.builder->CreateLoad(gep, ptrname);
    // G.setValuetoMap(ptrname, ptrload);
    // auto* ptype = llvm::cast<llvm::PointerType>(ptrload->getType());
    // if (ptype->getElementType()->isFirstClassType()) {
    G.setValuetoMap("ptr_" + newname, gep);
    llvm::Value* valload = G.builder->CreateLoad(gep, newname);
    G.setValuetoMap(newname, valload);
    // }
  }
}
void LLVMGenerator::CodeGenVisitor::operator()(FcallInst& i) {
  bool isclosure = i.ftype == CLOSURE;
  std::vector<llvm::Value*> args;
  auto m = G.variable_map[G.curfunc];
  for (auto& a : i.args) {
    // TODO(tomoya): need to add type infomation to argument...
    auto it = m->find(a);
    if (it != m->end()) {
      args.emplace_back(it->second);
    } else {
      args.emplace_back(G.findValue("ptr_" + a));
    }
  }

  llvm::Value* fun;
  switch (i.ftype) {
    case DIRECT:
      fun = getDirFun(i);
      break;
    case CLOSURE: {
      auto* cls = getClsFun(i);
      auto* capptr = G.builder->CreateStructGEP(cls, 1, i.fname + "_cap");
      auto* fnptr = G.builder->CreateStructGEP(cls, 0, i.fname + "_funptr");
      args.emplace_back(capptr);
      fun = G.builder->CreateLoad(fnptr, i.fname + "_fun");

    } break;
    case EXTERNAL:
      fun = getExtFun(i);
      break;
    default:
      break;
  }

  if (i.istimed) {  // if arguments is timed value, call addTask
    createAddTaskFn(i, isclosure, isglobal);
  } else {
    if (std::holds_alternative<types::Void>(i.type)) {
      G.builder->CreateCall(fun, args);
    } else {
      auto res = G.builder->CreateCall(fun, args, i.lv_name);
      G.setValuetoMap(i.lv_name, res);
    }
  }
}
llvm::Value* LLVMGenerator::CodeGenVisitor::getDirFun(FcallInst& i) {
  auto fun = G.module->getFunction(i.fname);
  if (fun == nullptr) {
    throw std::logic_error("function could not be referenced");
  }
  return fun;
}
llvm::Value* LLVMGenerator::CodeGenVisitor::getClsFun(FcallInst& i) {
  auto fptr = G.findValue(i.fname);
  return fptr;
}
llvm::Value* LLVMGenerator::CodeGenVisitor::getExtFun(FcallInst& i) {
  auto it = LLVMBuiltin::ftable.find(i.fname);
  if (it == LLVMBuiltin::ftable.end()) {
    throw std::runtime_error("could not find external function \"" + i.fname +
                             "\"");
  }
  BuiltinFnInfo& fninfo = it->second;
  auto* fntype = llvm::cast<llvm::FunctionType>(G.getType(fninfo.mmmtype));
  auto fn = G.module->getOrInsertFunction(fninfo.target_fnname, fntype);
  auto f = llvm::cast<llvm::Function>(fn.getCallee());
  f->setCallingConv(llvm::CallingConv::C);
  return f;
}

void LLVMGenerator::CodeGenVisitor::createAddTaskFn(FcallInst& i,
                                                    const bool isclosure,
                                                    const bool isglobal) {
  auto tv = G.findValue("ptr_" + i.args[0]);

  auto timeptr = G.builder->CreateStructGEP(tv, 0);
  auto timeval = G.builder->CreateLoad(timeptr);
  auto valptr = G.builder->CreateStructGEP(tv, 1);
  auto val = G.builder->CreateLoad(valptr);

  auto targetfn = G.module->getFunction(i.fname);
  auto ptrtofn =
      llvm::ConstantExpr::getBitCast(targetfn, G.builder->getInt8PtrTy());
  // auto taskid = taskfn_typeid++;
  // tasktype_list.emplace_back(i.type);

  std::vector<llvm::Value*> args = {timeval, ptrtofn, val};
  llvm::Value* addtask_fn;
  auto globalspace = G.variable_map[G.mainentry->getParent()];
  if (isclosure) {
    llvm::Value* clsptr = (isglobal) ? G.findValue(i.fname + "_cap")
                                     : G.findValue("clsarg_" + i.fname);

    auto* upcasted =
        G.builder->CreateBitCast(clsptr, G.builder->getInt8PtrTy());
    G.setValuetoMap(i.fname + "_cls_i8", upcasted);
    args.push_back(upcasted);

    addtask_fn = globalspace->find("addTask_cls")->second;
  } else {
    addtask_fn = globalspace->find("addTask")->second;
  }
  auto* res = G.builder->CreateCall(addtask_fn, args);
  G.setValuetoMap(i.lv_name, res);
}

void LLVMGenerator::CodeGenVisitor::operator()(
    MakeClosureInst& i) {  // store the capture address to memory
  auto* closuretype = G.getType(G.typeenv.find(i.lv_name));
  auto targetf = G.module->getFunction(i.fname);
  auto closureptrname = i.fname + "_cls";
  // always heap allocation!
  auto* closure_ptr =
      createAllocation(true, closuretype, nullptr, closureptrname);
  auto* fun_ptr = G.builder->CreateStructGEP(closure_ptr, 0, "fun_ptr");
  G.builder->CreateStore(targetf, fun_ptr);
  auto* capture_ptr = G.builder->CreateStructGEP(closure_ptr, 1, "capture_ptr");
  unsigned int idx = 0;
  for (auto& cap : i.captures) {
    llvm::Value* fv = G.tryfindValue(cap);
    auto gep = G.builder->CreateStructGEP(capture_ptr, idx, "capture_" + cap);
    G.builder->CreateStore(fv, gep);
    idx += 1;
  }
  // G.setValuetoMap(capptrname, capture_ptr);  // no need?
  G.setValuetoMap(closureptrname, closure_ptr);
  G.cls_to_funmap[i.lv_name] = G.module->getFunction(i.fname);  // no need?
}
void LLVMGenerator::CodeGenVisitor::operator()(ArrayInst& i) {}
void LLVMGenerator::CodeGenVisitor::operator()(ArrayAccessInst& i) {}
void LLVMGenerator::CodeGenVisitor::operator()(IfInst& i) {}
void LLVMGenerator::CodeGenVisitor::operator()(ReturnInst& i) {
  auto v = G.tryfindValue(i.val);
  if (v == nullptr) {
    // case of returning function;
    v = G.tryfindValue(i.val + "_cls");
  }
  G.builder->CreateRet(v);
}
llvm::Value* LLVMGenerator::tryfindValue(std::string name) {
  auto map = variable_map[curfunc];
  auto res = map->find(name);
  return (res == map->end()) ? nullptr : res->second;
}
llvm::Value* LLVMGenerator::findValue(std::string name) {
  auto* res = tryfindValue(name);
  if (res == nullptr) {
    throw std::runtime_error("variable " + name +
                             " cannot be found in llvm conversion");
  }
  return res;
}

void LLVMGenerator::setValuetoMap(std::string name, llvm::Value* val) {
  auto map = variable_map[curfunc];
  map->try_emplace(name, val);
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