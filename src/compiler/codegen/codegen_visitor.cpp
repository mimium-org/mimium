/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/codegen/codegen_visitor.hpp"

namespace mimium {

const std::unordered_map<OP_ID, std::string> CodeGenVisitor::opid_to_ffi = {
    // names are declared in ffi.cpp
    {OP_ID::EXP, "pow"},  {OP_ID::MOD, "fmod"},      {OP_ID::GE, "ge"},
    {OP_ID::LE, "le"},    {OP_ID::GT, "gt"},         {OP_ID::LT, "lt"},
    {OP_ID::AND, "and"},  {OP_ID::BITAND, "and"},    {OP_ID::OR, "or"},
    {OP_ID::BITOR, "or"}, {OP_ID::LSHIFT, "lshift"}, {OP_ID::RSHIFT, "rshift"},
};

// Creates Allocation instruction or call malloc function depends on context
CodeGenVisitor::CodeGenVisitor(LLVMGenerator& g) : G(g), isglobal(false) {}

llvm::Value* CodeGenVisitor::createAllocation(bool isglobal, llvm::Type* type,
                                              llvm::Value* arraysize = nullptr,
                                              const llvm::Twine& name = "") {
  llvm::Value* res = nullptr;
  llvm::Type* t = type;
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
    res = G.builder->CreateAlloca(type, arraysize, "ptr_" + name);
  }
  return res;
};
// Create StoreInst if storing to already allocated value
bool CodeGenVisitor::createStoreOw(std::string varname,
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

void CodeGenVisitor::operator()(NumberInst& i) {
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
void CodeGenVisitor::operator()(AllocaInst& i) {
  auto ptrname = "ptr_" + i.lv_name;
  auto* type = G.getType(i.lv_name);
  auto* ptr = createAllocation(isglobal, type, nullptr, i.lv_name);
  G.setValuetoMap(ptrname, ptr);
}
void CodeGenVisitor::operator()(RefInst& i) {
  auto ptrname = "ptr_" + i.lv_name;
  auto ptrptrname = "ptr_" + ptrname;
  auto* ptrtoptr =
      createAllocation(isglobal, G.getType(i.type), nullptr, ptrname);
  G.builder->CreateStore(G.findValue(ptrname), ptrtoptr);
  auto* ptr = G.builder->CreateLoad(ptrtoptr, ptrptrname);
  G.setValuetoMap(ptrname, ptr);
  G.setValuetoMap(ptrptrname, ptrtoptr);
}
void CodeGenVisitor::operator()(AssignInst& i) {
  if (types::isPrimitive(i.type)) {
    // copy assignment
    auto ptr = G.findValue("ptr_" + i.lv_name);
    G.builder->CreateStore(G.findValue(i.val), ptr);
    auto* newval = G.builder->CreateLoad(ptr, i.lv_name);
    G.setValuetoMap(i.lv_name, newval);
  } else {
  }
}
void CodeGenVisitor::operator()(TimeInst& i) {
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

void CodeGenVisitor::operator()(OpInst& i) {
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
    default: {
      auto id = i.getOPid();
      if (opid_to_ffi.count(id)) {
        auto fname = opid_to_ffi.find(id)->second;
        retvalue = G.builder->CreateCall(G.getForeignFunction(fname),
                                         {lhs, rhs}, i.lv_name);
      } else {
        retvalue = G.builder->CreateUnreachable();
      }
      break;
    }
  }
  G.setValuetoMap(i.lv_name, retvalue);
}
void CodeGenVisitor::operator()(FunInst& i) {
  auto memobj = G.memobjcoll.getAliasFromMap(i.lv_name);

  auto* ft = createFunctionType(i, memobj);
  auto* f = createFunction(ft, i);

  G.curfunc = f;
  G.variable_map.emplace(f, std::make_shared<LLVMGenerator::namemaptype>());
  G.createNewBasicBlock("entry", f);

  addArgstoMap(f, i, memobj);
  context_hasself = (i.hasself) ? "ptr_" + i.lv_name + ".self.mem" : "";

  for (auto& cinsts : i.body->instructions) {
    G.visitInstructions(cinsts, false);
  }

  if (G.currentblock->getTerminator() == nullptr &&
      ft->getReturnType()->isVoidTy()) {
    G.builder->CreateRetVoid();
  }
  G.switchToMainFun();
}
llvm::FunctionType* CodeGenVisitor::createFunctionType(
    FunInst& i, std::optional<types::Alias>& memobj) {
  bool hasfv = !i.freevariables.empty();
  llvm::FunctionType* ft;
  if (hasfv || i.lv_name == "dsp") {
    auto& type = G.typeenv.find(i.lv_name + "_cls");
    auto clsty = llvm::cast<llvm::StructType>(G.getType(type));
    auto funty = llvm::cast<llvm::PointerType>(clsty->getElementType(0))
                     ->getElementType();
    ft = llvm::cast<llvm::FunctionType>(funty);
  } else {
    ft = llvm::cast<llvm::FunctionType>(G.getType(G.typeenv.find(i.lv_name)));
  }
  // add memory object such as self
  if (memobj) {
    auto reftype = types::Ref(memobj.value());
    auto* memobjtype = G.typeconverter(reftype);

    std::vector<llvm::Type*> newparams = ft->params();
    // if(i.lv_name!="dsp"){
    // newparams.pop_back();
    // }
    newparams.push_back(memobjtype);
    ft = llvm::FunctionType::get(ft->getReturnType(), newparams, false);
  }
  return ft;
}

llvm::Function* CodeGenVisitor::createFunction(llvm::FunctionType* type,
                                               FunInst& i) {
  auto link = llvm::Function::ExternalLinkage;
  auto* f = llvm::Function::Create(type, link, i.lv_name, *G.module);
  G.setValuetoMap(i.lv_name, f);
  return f;
}
void CodeGenVisitor::addArgstoMap(llvm::Function* f, FunInst& i,
                                  std::optional<types::Alias>& memobj) {
  // arguments are [actual arguments], capture , memobjs
  auto f_it = std::begin(f->args());
  for (auto& a : i.args) {
    f_it->setName(a);
    G.setValuetoMap(a, f_it++);
  }
  auto lastelem = std::prev(f->args().end());
  if (memobj) {
    auto name = i.lv_name + ".mem";
    lastelem->setName(name);
    G.setValuetoMap(name, lastelem);
    setMemObjsToMap(i, lastelem);
    lastelem = std::prev(lastelem);
  }
  bool hasfv = !i.freevariables.empty() || i.lv_name == "dsp";
  if (hasfv) {
    auto name = "clsarg_" + i.lv_name;
    lastelem->setName(name);
    G.setValuetoMap(name, lastelem);
    setFvsToMap(i, lastelem);
    if (i.isrecursive) {
      G.setValuetoMap(i.lv_name + "_cls", f);
    }
  }
}

void CodeGenVisitor::setFvsToMap(FunInst& i, llvm::Value* clsarg) {
  auto* lastarg = clsarg;
  for (int id = 0; id < i.freevariables.size(); id++) {
    std::string newname = i.freevariables[id];
    auto* gep = G.builder->CreateStructGEP(lastarg, id, "fv");
    auto ptrname = "ptr_" + newname;
    auto* ptrload = G.builder->CreateLoad(gep, ptrname);
    G.setValuetoMap(ptrname, ptrload);
    auto* ptype = llvm::cast<llvm::PointerType>(ptrload->getType());
    if (ptype->getElementType()->isFirstClassType()) {
    // G.setValuetoMap("ptr_" + newname, gep);
    llvm::Value* valload = G.builder->CreateLoad(ptrload, newname);
    G.setValuetoMap(newname, valload);
    }
  }
}
void CodeGenVisitor::setMemObjsToMap(FunInst& i, llvm::Value* memarg) {
  auto* lastarg = memarg;
  for (int id = 0; id < i.memory_objects.size(); id++) {
    std::string newname = i.memory_objects[id] + ".mem";
    auto* gep = G.builder->CreateStructGEP(lastarg, id, "memobj");
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
void CodeGenVisitor::operator()(FcallInst& i) {
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
  if (G.memobjcoll.getAliasFromMap(i.fname)) {
    args.emplace_back(G.findValue("ptr_" + i.fname + ".mem"));
  }

  llvm::Value* fun;
  switch (i.ftype) {
    case DIRECT:
      fun = getDirFun(i);
      break;
    case CLOSURE: {
      auto* cls = getClsFun(i);
      if (llvm::cast<llvm::PointerType>(cls->getType())
              ->getElementType()
              ->isFunctionTy()) {
        fun = cls;
      } else {
        auto* capptr = G.builder->CreateStructGEP(cls, 1, i.fname + "_cap");
        auto* fnptr = G.builder->CreateStructGEP(cls, 0, i.fname + "_funptr");
        args.emplace_back(capptr);
        fun = G.builder->CreateLoad(fnptr, i.fname + "_fun");
      }
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
llvm::Value* CodeGenVisitor::getDirFun(FcallInst& i) {
  auto fun = G.module->getFunction(i.fname);
  fun->setLinkage(llvm::GlobalValue::InternalLinkage);
  if (fun == nullptr) {
    throw std::logic_error("function could not be referenced");
  }
  return fun;
}
llvm::Value* CodeGenVisitor::getClsFun(FcallInst& i) {
  auto fptr = G.findValue(i.fname + "_cls");
  return fptr;
}
llvm::Value* CodeGenVisitor::getExtFun(FcallInst& i) {
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

void CodeGenVisitor::createAddTaskFn(FcallInst& i, const bool isclosure,
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
    llvm::Value* clsptr = (isglobal) ? G.findValue(i.fname + "_cls_capture_ptr")
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

void CodeGenVisitor::operator()(
    MakeClosureInst& i) {  // store the capture address to memory
  auto* closuretype = G.getType(G.typeenv.find(i.lv_name));
  auto targetf = G.module->getFunction(i.fname);
  auto closureptrname = i.fname + "_cls";
  // always heap allocation!
  auto* closure_ptr =
      createAllocation(true, closuretype, nullptr, closureptrname);
  auto* fun_ptr =
      G.builder->CreateStructGEP(closure_ptr, 0, i.lv_name + "_fun_ptr");
  G.builder->CreateStore(targetf, fun_ptr);
  auto* capture_ptr =
      G.builder->CreateStructGEP(closure_ptr, 1, i.lv_name + "_capture_ptr");
  unsigned int idx = 0;
  for (auto& cap : i.captures) {
    llvm::Value* fv = G.tryfindValue("ptr_"+cap);
    auto gep = G.builder->CreateStructGEP(capture_ptr, idx, "capture_" + cap);
    G.builder->CreateStore(fv, gep);
    idx += 1;
  }
  G.setValuetoMap(i.lv_name + "_capture_ptr", capture_ptr);
  G.setValuetoMap(closureptrname, closure_ptr);
}
void CodeGenVisitor::operator()(ArrayInst& i) {}
void CodeGenVisitor::operator()(ArrayAccessInst& i) {}
void CodeGenVisitor::operator()(IfInst& i) {}
void CodeGenVisitor::operator()(ReturnInst& i) {
  auto v = G.tryfindValue(i.val);

  if (v == nullptr) {
    // case of returning function;
    v = G.tryfindValue(i.val + "_cls");
  }
  if (!context_hasself.empty()) {
    auto selfv = G.tryfindValue(context_hasself);
    if (selfv != nullptr) {
      G.builder->CreateStore(v, selfv);
    }
  }
  G.builder->CreateRet(v);
}
}  // namespace mimium