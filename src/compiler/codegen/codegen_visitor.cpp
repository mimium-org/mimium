/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/codegen/codegen_visitor.hpp"

namespace mimium {
using OpId = ast::OpId;
const std::unordered_map<OpId, std::string> CodeGenVisitor::opid_to_ffi = {
    // names are declared in ffi.cpp

    {OpId::Exponent, "pow"}, {OpId::Mod, "fmod"},       {OpId::GreaterEq, "ge"},
    {OpId::LessEq, "le"},    {OpId::GreaterThan, "gt"}, {OpId::LessThan, "lt"},
    {OpId::And, "and"},      {OpId::BitAnd, "and"},     {OpId::Or, "or"},
    {OpId::BitOr, "or"},     {OpId::LShift, "lshift"},  {OpId::RShift, "rshift"},
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
    const int bitsize = 64;
    auto* sizeinst = llvm::ConstantInt::get(G.ctx, llvm::APInt(bitsize, size, false));
    auto* rawres =
        G.builder->CreateCall(G.module->getFunction("mimium_malloc"), {sizeinst}, rawname);
    res = G.builder->CreatePointerCast(rawres, llvm::PointerType::get(t, 0), "ptr_" + name);
    G.setValuetoMap(rawname, rawres);
  } else {
    res = G.builder->CreateAlloca(type, arraysize, "ptr_" + name);
  }
  return res;
};
// Create StoreInst if storing to already allocated value
bool CodeGenVisitor::createStoreOw(std::string varname, llvm::Value* val_to_store) {
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

void CodeGenVisitor::operator()(minst::Number& i) {
  auto* finst = llvm::ConstantFP::get(G.ctx, llvm::APFloat(i.val));
  auto* ptr = G.tryfindValue("ptr_" + i.lv_name);
  if (ptr != nullptr) {  // case of temporary value
    G.builder->CreateStore(finst, ptr);
    auto* res = G.builder->CreateLoad(ptr, i.lv_name);
    G.setValuetoMap(i.lv_name, res);
  } else {
    G.setValuetoMap(i.lv_name, finst);
  }
}
void CodeGenVisitor::operator()(minst::String& i) {
  auto* cstr = llvm::ConstantDataArray::getString(G.ctx, i.val);
  auto* i8ptrty = G.builder->getInt8PtrTy();
  auto* gvalue =
      llvm::cast<llvm::GlobalVariable>(G.module->getOrInsertGlobal(i.val, cstr->getType()));
  gvalue->setInitializer(cstr);
  gvalue->setLinkage(llvm::GlobalValue::InternalLinkage);
  gvalue->setName("str");
  auto* bitcast = G.builder->CreateBitCast(gvalue, i8ptrty);
  // auto zero =
  // llvm::ConstantInt::get(G.builder->getInt64Ty(), llvm::APInt(64, 0));
  // auto strptr = G.builder->CreateInBoundsGEP(gvalue, {zero, zero}, "gep");
  // auto strptr = G.builder->CreateLoad(gvalue,i.lv_name);
  G.setValuetoMap(i.lv_name, bitcast);
}
void CodeGenVisitor::operator()(minst::Allocate& i) {
  // TODO(tomoya) Is a type value for AllocaInst no longer needed?
  auto ptrname = "ptr_" + i.lv_name;
  auto* type = G.getType(i.lv_name);
  auto* ptr = createAllocation(isglobal, type, nullptr, i.lv_name);
  G.setValuetoMap(ptrname, ptr);
}
void CodeGenVisitor::operator()(minst::Ref& i) {  // temporarily unused
  // auto ptrname = "ptr_" + i.lv_name;
  // auto ptrptrname = "ptr_" + ptrname;
  // auto* ptrtoptr =
  //     createAllocation(isglobal, G.getType(i.type), nullptr, ptrname);
  // G.builder->CreateStore(G.findValue(ptrname), ptrtoptr);
  // auto* ptr = G.builder->CreateLoad(ptrtoptr, ptrptrname);
  // G.setValuetoMap(ptrname, ptr);
  // G.setValuetoMap(ptrptrname, ptrtoptr);
}
void CodeGenVisitor::operator()(minst::Load& i) {
  if (types::isPrimitive(i.type)) {
    // copy assignment
    auto* ptr = G.findValue("ptr_" + i.lv_name);
    G.builder->CreateStore(G.findValue(i.val), ptr);
    auto* newval = G.builder->CreateLoad(ptr, i.lv_name);
    // G.setValuetoMap(i.lv_name, newval);
    G.variable_map[G.curfunc]->insert_or_assign(i.lv_name,
                                                newval);  // force overwrite
    G.addOverWrittenVar(i.lv_name);
  } else {
  }
}

void CodeGenVisitor::operator()(minst::Store& i) {
}

void CodeGenVisitor::operator()(minst::Op& i) {
  if (i.lhs.empty()) {
    createUniOp(i);
  } else {
    createBinOp(i);
  }
}

void CodeGenVisitor::createUniOp(minst::Op& i) {
  llvm::Value* retvalue = nullptr;
  auto* rhs = G.findValue(i.rhs);
  switch (i.op) {
    case ast::OpId::Sub:
      retvalue = G.builder->CreateUnOp(llvm::Instruction::UnaryOps::FNeg, rhs, i.lv_name);
      break;
    case ast::OpId::Not:
      retvalue = G.builder->CreateCall(G.getForeignFunction("not"), {rhs}, i.lv_name);
      break;
    default: retvalue = G.builder->CreateUnreachable(); break;
  }
  G.setValuetoMap(i.lv_name, retvalue);
}

void CodeGenVisitor::createBinOp(minst::Op& i) {
  llvm::Value* retvalue = nullptr;
  auto* lhs = G.findValue(i.lhs);
  auto* rhs = G.findValue(i.rhs);
  switch (i.op) {
    case ast::OpId::Add: retvalue = G.builder->CreateFAdd(lhs, rhs, i.lv_name); break;
    case ast::OpId::Sub: retvalue = G.builder->CreateFSub(lhs, rhs, i.lv_name); break;
    case ast::OpId::Mul: retvalue = G.builder->CreateFMul(lhs, rhs, i.lv_name); break;
    case ast::OpId::Div: retvalue = G.builder->CreateFDiv(lhs, rhs, i.lv_name); break;
    default: {
      if (opid_to_ffi.count(i.op) > 0) {
        auto fname = opid_to_ffi.find(i.op)->second;
        retvalue = G.builder->CreateCall(G.getForeignFunction(fname), {lhs, rhs}, i.lv_name);
      } else {
        retvalue = G.builder->CreateUnreachable();
      }
      break;
    }
  }
  G.setValuetoMap(i.lv_name, retvalue);
}
void CodeGenVisitor::operator()(minst::Function& i) {
  bool hascapture = G.cc.hasCapture(i.lv_name);
  bool hasmemobj = G.funobj_map.count(i.lv_name) > 0;
  if (hasmemobj) { context_hasself = G.funobj_map.at(i.lv_name)->hasself; }
  auto* ft = createFunctionType(i, hascapture, hasmemobj);
  auto* f = createFunction(ft, i);

  G.curfunc = f;
  G.variable_map.emplace(f, std::make_shared<LLVMGenerator::namemaptype>());
  G.createNewBasicBlock("entry", f);

  addArgstoMap(f, i, hascapture, hasmemobj);
  if (i.isrecursive && hascapture) { G.setValuetoMap("ptr_" + i.lv_name + "_cls", f); }
  for (auto& cinsts : i.body->instructions) { G.visitInstructions(cinsts, false); }

  if (G.currentblock->getTerminator() == nullptr && ft->getReturnType()->isVoidTy()) {
    G.builder->CreateRetVoid();
  }
  G.switchToMainFun();
}
llvm::FunctionType* CodeGenVisitor::createFunctionType(minst::Function& i, bool hascapture,
                                                       bool hasmemobj) {
  auto mmmfntype = rv::get<types::Function>(G.typeenv.find(i.lv_name));
  auto& argtypes = mmmfntype.arg_types;

  if (hascapture || i.lv_name == "dsp") {
    auto captype = types::Ref{G.cc.getCaptureType(i.lv_name)};
    argtypes.emplace_back(std::move(captype));
    // auto clsty = llvm::cast<llvm::StructType>(G.getType(captype));
  }
  if (hasmemobj || i.lv_name == "dsp") {
    auto memobjtype = types::Ref{G.funobj_map.at(i.lv_name)->objtype};
    argtypes.emplace_back(std::move(memobjtype));
  }

  return llvm::cast<llvm::FunctionType>(G.typeconverter(mmmfntype));
}

llvm::Function* CodeGenVisitor::createFunction(llvm::FunctionType* type, minst::Function& i) {
  auto link = llvm::Function::ExternalLinkage;
  auto* f = llvm::Function::Create(type, link, i.lv_name, *G.module);
  G.setValuetoMap(i.lv_name, f);
  return f;
}
void CodeGenVisitor::addArgstoMap(llvm::Function* f, minst::Function& i, bool hascapture,
                                  bool hasmemobj) {
  // arguments are [actual arguments], capture , memobjs
  auto* arg = std::begin(f->args());
  for (auto& a : i.args) {
    arg->setName(a);
    G.setValuetoMap(a, arg);
    std::advance(arg, 1);
  }
  if (hascapture || i.lv_name == "dsp") {
    auto name = "ptr_" + i.lv_name + ".cap";
    arg->setName(name);
    G.setValuetoMap(name, arg);
    setFvsToMap(i, arg);
    std::advance(arg, 1);
  }
  if (hasmemobj || i.lv_name == "dsp") {
    auto name = i.lv_name + ".mem";
    arg->setName(name);
    G.setValuetoMap(name, arg);
    setMemObjsToMap(i, arg);
  }
}

void CodeGenVisitor::setMemObj(llvm::Value* memarg, std::string const& name, int index) {
  auto* gep = G.builder->CreateStructGEP(memarg, index, "ptr_" + name);
  G.setValuetoMap("ptr_" + name, gep);
  llvm::Value* valload = G.builder->CreateLoad(gep, name);
  G.setValuetoMap(name, valload);
}
void CodeGenVisitor::setMemObjsToMap(minst::Function& i, llvm::Value* memarg) {
  auto& fobjtree = G.funobj_map.at(i.lv_name);
  auto& memobjs = fobjtree->memobjs;
  int count = 0;
  // appending order matters! self should be put on last.
  for (auto& o : memobjs) {
    FunObjTree& obj = o;
    setMemObj(memarg, obj.fname + ".mem", count++);
  }
  if (fobjtree->hasself) { setMemObj(memarg, "self", count++); }
}

void CodeGenVisitor::setFvsToMap(minst::Function& i, llvm::Value* clsarg) {
  auto& captures = G.cc.getCaptureNames(i.lv_name);

  int count = 0;
  for (auto& cap : captures) {
    auto capname = cap;
    if (G.module->getFunction(cap) != nullptr) { capname = cap + "_cls"; }
    auto* gep = G.builder->CreateStructGEP(clsarg, count++, "fv");
    auto ptrname = "ptr_" + capname;
    auto* ptrload = G.builder->CreateLoad(gep, ptrname);
    G.setValuetoMap(ptrname, ptrload);
    auto* ptype = llvm::cast<llvm::PointerType>(ptrload->getType());
    if (ptype->getElementType()->isFirstClassType()) {
      auto* valload = G.builder->CreateLoad(ptrload, capname);
      G.setValuetoMap(capname, valload);
    }
  }
}

void CodeGenVisitor::operator()(minst::Fcall& i) {
  bool isclosure = i.ftype == CLOSURE;
  std::vector<llvm::Value*> args;
  auto m = G.variable_map[G.curfunc];
  for (auto& a : i.args) {
    auto* v = G.tryfindValue(a);
    if (v == nullptr) { v = G.findValue("ptr_" + a); }
    args.emplace_back(v);
  }
  if (G.cc.hasCapture(i.fname)) {
    // append memory address made by MakeClosureInst
    auto* cap = G.tryfindValue("ptr_" + i.fname + "_cls");
    if (cap == nullptr) {  //, or inherited from argument
      cap = G.tryfindValue("ptr_" + i.fname + ".cap");
    }
    args.emplace_back(cap);
  }
  if (G.funobj_map.count(i.fname) > 0) {
    args.emplace_back(G.findValue("ptr_" + i.fname + ".mem"));
  }

  llvm::Value* fun = nullptr;
  switch (i.ftype) {
    case DIRECT: fun = getDirFun(i); break;
    case CLOSURE: fun = getClsFun(i); break;
    case EXTERNAL: fun = getExtFun(i); break;
    default: break;
  }

  if (i.time) {  // if arguments is timed value, call addTask
    createAddTaskFn(i, isclosure, isglobal);
  } else {
    if (std::holds_alternative<types::Void>(G.typeenv.find(i.lv_name))) {
      G.builder->CreateCall(fun, args);
    } else {
      auto* res = G.builder->CreateCall(fun, args, i.lv_name);
      G.setValuetoMap(i.lv_name, res);
      auto* resptr = G.tryfindValue("ptr_" + i.lv_name);
      if (resptr != nullptr) { G.builder->CreateStore(res, resptr); }
    }
  }
}
llvm::Value* CodeGenVisitor::getDirFun(minst::Fcall& i) {
  auto* fun = G.module->getFunction(i.fname);
  fun->setLinkage(llvm::GlobalValue::InternalLinkage);
  if (fun == nullptr) { throw std::runtime_error("function could not be referenced"); }
  return fun;
}
llvm::Value* CodeGenVisitor::getClsFun(minst::Fcall& i) { return getDirFun(i); }
llvm::Value* CodeGenVisitor::getExtFun(minst::Fcall& i) {
  auto it = LLVMBuiltin::ftable.find(i.fname);
  if (it == LLVMBuiltin::ftable.end()) {
    throw std::runtime_error("could not find external function \"" + i.fname + "\"");
  }
  return G.getForeignFunction(i.fname);
}

void CodeGenVisitor::createAddTaskFn(minst::Fcall& i, const bool isclosure,
                                     const bool /*isglobal*/) {
  auto* i8ptrty = G.builder->getInt8PtrTy();
  auto* timeval = G.findValue(i.time.value());
  auto* targetfn = G.module->getFunction(i.fname);
  auto* ptrtofn = llvm::ConstantExpr::getBitCast(targetfn, i8ptrty);

  std::vector<llvm::Value*> args = {timeval, ptrtofn};
  for (auto& a : i.args) { args.emplace_back(G.findValue(a)); }
  if (i.args.empty()) {
    auto* zero = llvm::ConstantFP::get(G.ctx, llvm::APFloat(0.0));
    args.emplace_back(zero);
  }

  llvm::Value* addtask_fn = nullptr;
  auto globalspace = G.variable_map[G.mainentry->getParent()];
  if (isclosure) {
    auto* capptr = G.tryfindValue("ptr_" + i.fname + ".cap");
    // fixme
    if (capptr == nullptr) { capptr = G.tryfindValue("ptr_" + i.fname + "_cls"); }
    args.push_back(G.builder->CreateBitCast(capptr, i8ptrty));

    addtask_fn = globalspace->find("addTask_cls")->second;
  } else {
    addtask_fn = globalspace->find("addTask")->second;
  }
  auto* res = G.builder->CreateCall(addtask_fn, args);
  G.setValuetoMap(i.lv_name, res);
}

void CodeGenVisitor::operator()(minst::MakeClosure& i) {  // store the capture address to memory
  auto& capturenames = G.cc.getCaptureNames(i.fname);
  auto* closuretype = G.getType(G.cc.getCaptureType(i.fname));

  auto* targetf = G.module->getFunction(i.fname);

  auto captureptrname = "ptr_" + i.fname + ".cap";
  // always heap allocation!
  auto* capture_ptr = createAllocation(true, closuretype, nullptr, captureptrname);
  // auto* fun_ptr =
  //     G.builder->CreateStructGEP(closure_ptr, 0, i.lv_name + "_fun_ptr");
  // G.builder->CreateStore(targetf, fun_ptr);
  // auto* capture_ptr =
  //     G.builder->CreateStructGEP(closure_ptr, 1, i.lv_name +
  //     "_capture_ptr");
  unsigned int idx = 0;
  for (auto& cap : capturenames) {
    llvm::Value* fv = G.tryfindValue("ptr_" + cap);
    if (fv == nullptr) { fv = G.findValue("ptr_" + cap + ".cap"); }
    auto* gep = G.builder->CreateStructGEP(capture_ptr, idx, "capture_" + cap);
    G.builder->CreateStore(fv, gep);
    idx += 1;
  }
  // G.setValuetoMap(i.lv_name + "_capture_ptr", capture_ptr);
  G.setValuetoMap(captureptrname, capture_ptr);
  // G.setValuetoMap("ptr_" + closureptrname, closure_ptr);
}
void CodeGenVisitor::operator()(minst::Array& i) {}
// void CodeGenVisitor::operator()(minst::ArrayAccess& i) {
//   auto* v = G.tryfindValue(i.name);
//   auto* indexfloat = G.tryfindValue(i.index);
//   auto* dptrtype = llvm::PointerType::get(G.builder->getDoubleTy(), 0);
//   auto* arraccessfun = G.module->getFunction("access_array_lin_interp");
//   // auto indexint =
//   // G.builder->CreateBitCast(indexfloat,G.builder->getInt64Ty());
//   // const int bitsize = 64;
//   // auto* zero = llvm::ConstantInt::get(G.builder->getInt64Ty(), llvm::APInt(bitsize, 0));
//   // auto gep =
//   // G.builder->CreateInBoundsGEP(dptrtype,v,{indexint,zero},"arrayaccess");
//   // auto load = G.builder->CreateLoad(gep,"arraccessload");
//   // G.setValuetoMap("ptr_"+i.lv_name, gep);
//   // G.setValuetoMap(i.lv_name, load);
//   auto* res = G.builder->CreateCall(arraccessfun, {v, indexfloat}, "arrayaccess");
//   G.setValuetoMap(i.lv_name, res);
// }
void CodeGenVisitor::operator()(minst::Field& i) {
  auto* v = G.findValue(i.name);
  if (auto* index = std::get_if<int>(&i.index)) {
    auto* tupleptrtype = llvm::PointerType::get(v->getType(), 0);
    G.builder->CreateInBoundsGEP(v, {G.getConstInt(*index), G.getZero()}, "tupleaccess");
  }else{
    //todo:struct access;
  }
}

void CodeGenVisitor::createIfBody(mir::blockptr& block, llvm::Value* ret_ptr) {
  auto& insts = block->instructions;
  if (ret_ptr == nullptr) {
    for (auto& ti : insts) { std::visit(*this, ti); }
  } else {
    if (!std::holds_alternative<minst::ReturnInst>(s.back())) {
      throw std::logic_error("non-void block should have minst::Return for last line");
    }
    for (auto&& iter = std::begin(insts); iter != std::prev(std::end(insts)); iter++) {
      std::visit(*this, *iter);
    }
    auto& retinst = std::get<minst::ReturnInst>(s.back());
    G.builder->CreateStore(G.findValue(retinst.val), ret_ptr);
  }
}

void CodeGenVisitor::operator()(minst::If& i) {
  auto* thisbb = G.builder->GetInsertBlock();
  auto* cond = G.findValue(i.cond);
  auto* cmp = G.builder->CreateFCmpOGT(cond, llvm::ConstantFP::get(G.ctx, llvm::APFloat(0.0)));
  auto* endbb = llvm::BasicBlock::Create(G.ctx, i.lv_name + "_end", G.curfunc);
  llvm::Value* retptr = G.tryfindValue("ptr_" + i.lv_name);

  auto* thenbb = llvm::BasicBlock::Create(G.ctx, i.lv_name + "_then", G.curfunc);
  G.builder->SetInsertPoint(thenbb);
  createIfBody(i.thenblock, retptr);
  G.builder->CreateBr(endbb);
  auto* elsebb = llvm::BasicBlock::Create(G.ctx, i.lv_name + "_else", G.curfunc);
  G.builder->SetInsertPoint(elsebb);
  if (i.elseblock.has_value()) { createIfBody(i.elseblock.value(), retptr); }
  G.builder->CreateBr(endbb);
  G.builder->SetInsertPoint(endbb);
  auto& ifrettype = G.typeenv.find(i.lv_name);
  bool isvoid = std::holds_alternative<types::Void>(ifrettype);
  if (!isvoid) {
    auto* retval = G.builder->CreateLoad(retptr, i.lv_name);
    G.setValuetoMap(i.lv_name, retval);
  }

  G.builder->SetInsertPoint(thisbb);
  G.builder->CreateCondBr(cmp, thenbb, elsebb);
  G.builder->SetInsertPoint(endbb);
}
void CodeGenVisitor::operator()(minst::Return& i) {
  auto* res = G.tryfindValue(i.val);
  if (res == nullptr) {
    // case of returning function;
    res = G.tryfindValue(i.val + "_cls");
  }
  // store self value
  if (context_hasself) {
    auto* selfv = G.tryfindValue("ptr_self");
    G.builder->CreateStore(res, selfv);
  }
  G.builder->CreateRet(res);
}
}  // namespace mimium