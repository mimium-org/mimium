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

llvm::Value* CodeGenVisitor::visit(mir::valueptr val) {
  return std::visit(overloaded{
                        [&](mir::Instructions& inst) { return std::visit(*this, inst); },
                        [](mir::Constants& c) -> llvm::Value* { return nullptr; },       // TODO
                        [](mir::Argument& a) -> llvm::Value* { return nullptr; },        // TODO
                        [](mir::ExternalSymbol& a) -> llvm::Value* { return nullptr; },  // TODO
                        [](mir::Self& a) -> llvm::Value* { return nullptr; },            // TODO

                    },
                    *val);
}

void CodeGenVisitor::registerLlvmVal(mir::valueptr mirval, llvm::Value* llvmval) {
  mir_to_llvm.emplace(mirval, llvmval);
}
llvm::Value* CodeGenVisitor::getLlvmVal(mir::valueptr mirval) {
  auto iter = mir_to_llvm.find(mirval);
  assert(iter == mir_to_llvm.end());
  return iter->second;
}

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

llvm::Value* CodeGenVisitor::operator()(minst::Number& i) {
  return llvm::ConstantFP::get(G.ctx, llvm::APFloat(i.val));
}
llvm::Value* CodeGenVisitor::operator()(minst::String& i) {
  auto* cstr = llvm::ConstantDataArray::getString(G.ctx, i.val);
  auto* i8ptrty = G.builder->getInt8PtrTy();
  auto* gvalue =
      llvm::cast<llvm::GlobalVariable>(G.module->getOrInsertGlobal(i.val, cstr->getType()));
  gvalue->setInitializer(cstr);
  gvalue->setLinkage(llvm::GlobalValue::InternalLinkage);
  gvalue->setName("str");
  auto* bitcast = G.builder->CreateBitCast(gvalue, i8ptrty);
  return bitcast;
}
llvm::Value* CodeGenVisitor::operator()(minst::Allocate& i) {
  // TODO(tomoya) Is a type value for AllocaInst no longer needed?
  auto* ptr = createAllocation(isglobal, G.getType(i.type), nullptr, i.name);
  return ptr;
}
llvm::Value* CodeGenVisitor::operator()(minst::Ref& i) {  // temporarily unused
  // auto ptrname = "ptr_" + i.lv_name;
  // auto ptrptrname = "ptr_" + ptrname;
  // auto* ptrtoptr =
  //     createAllocation(isglobal, G.getType(i.type), nullptr, ptrname);
  // G.builder->CreateStore(G.findValue(ptrname), ptrtoptr);
  // auto* ptr = G.builder->CreateLoad(ptrtoptr, ptrptrname);
  // G.setValuetoMap(ptrname, ptr);
  // G.setValuetoMap(ptrptrname, ptrtoptr);
}
llvm::Value* CodeGenVisitor::operator()(minst::Load& i) {
  auto* target = getLlvmVal(i.target);
  return G.builder->CreateLoad(target, i.name);
}

llvm::Value* CodeGenVisitor::operator()(minst::Store& i) {
  auto* target = getLlvmVal(i.target);
  auto* val = getLlvmVal(i.value);
  return G.builder->CreateStore(val, target, false);
}

llvm::Value* CodeGenVisitor::operator()(minst::Op& i) {
  if (!i.lhs.has_value()) { return createUniOp(i); }
  return createBinOp(i);
}

llvm::Value* CodeGenVisitor::createUniOp(minst::Op& i) {
  llvm::Value* retvalue = nullptr;
  auto* rhs = getLlvmVal(i.rhs);
  switch (i.op) {
    case ast::OpId::Sub:
      return G.builder->CreateUnOp(llvm::Instruction::UnaryOps::FNeg, rhs, i.name);
      break;
    case ast::OpId::Not:
      return G.builder->CreateCall(G.getForeignFunction("not"), {rhs}, i.name);
      break;
    default: return G.builder->CreateUnreachable(); break;
  }
}

llvm::Value* CodeGenVisitor::createBinOp(minst::Op& i) {
  llvm::Value* retvalue = nullptr;
  auto* lhs = getLlvmVal(i.lhs.value());
  auto* rhs = getLlvmVal(i.rhs);
  switch (i.op) {
    case ast::OpId::Add: return G.builder->CreateFAdd(lhs, rhs, i.name); break;
    case ast::OpId::Sub: return G.builder->CreateFSub(lhs, rhs, i.name); break;
    case ast::OpId::Mul: return G.builder->CreateFMul(lhs, rhs, i.name); break;
    case ast::OpId::Div: return G.builder->CreateFDiv(lhs, rhs, i.name); break;
    default: {
      if (opid_to_ffi.count(i.op) > 0) {
        auto fname = opid_to_ffi.find(i.op)->second;
        return G.builder->CreateCall(G.getForeignFunction(fname), {lhs, rhs}, i.name);
      }

      return G.builder->CreateUnreachable();
      break;
    }
  }
}
llvm::Value* CodeGenVisitor::operator()(minst::Function& i) {
  bool hascapture = !i.freevariables.empty();
  bool hasmemobj = !i.memory_objects.empty();
  if (hasmemobj) { context_hasself = i.hasself; }
  auto* ft = createFunctionType(i, hascapture, hasmemobj);
  auto* f = createFunction(ft, i);

  G.curfunc = f;
  G.variable_map.emplace(f, std::make_shared<LLVMGenerator::namemaptype>());
  G.createNewBasicBlock("entry", f);

  addArgstoMap(f, i, hascapture, hasmemobj);

  for (auto& cinsts : i.body->instructions) { G.visitInstructions(cinsts, false); }

  if (G.currentblock->getTerminator() == nullptr && ft->getReturnType()->isVoidTy()) {
    G.builder->CreateRetVoid();
  }
  G.switchToMainFun();
  return f;
}
llvm::FunctionType* CodeGenVisitor::createFunctionType(minst::Function& i, bool hascapture,
                                                       bool hasmemobj) {
  auto& mmmfntype = rv::get<types::Function>(i.type);
  auto& argtypes = mmmfntype.arg_types;

  if (hascapture || i.name == "dsp") {
    auto captype = types::Ref{G.cc.getCaptureType(i.name)};
    argtypes.emplace_back(std::move(captype));
    // auto clsty = llvm::cast<llvm::StructType>(G.getType(captype));
  }
  if (hasmemobj || i.name == "dsp") {
    auto memobjtype = types::Ref{G.funobj_map.at(i.name)->objtype};
    argtypes.emplace_back(std::move(memobjtype));
  }

  return llvm::cast<llvm::FunctionType>(G.typeconverter(mmmfntype));
}

llvm::Function* CodeGenVisitor::createFunction(llvm::FunctionType* type, minst::Function& i) {
  auto link = llvm::Function::ExternalLinkage;
  auto* f = llvm::Function::Create(type, link, i.name, *G.module);
  G.setValuetoMap(i.name, f);
  return f;
}
void CodeGenVisitor::addArgstoMap(llvm::Function* f, minst::Function& i, bool hascapture,
                                  bool hasmemobj) {
  // arguments are [actual arguments], capture , memobjs
  auto* arg = std::begin(f->args());
  for (auto& a : i.args) {
    arg->setName(a->name);
    registerLlvmVal(std::make_shared<mir::Value>(*a), arg);
    std::advance(arg, 1);
  }
  if (hascapture || i.name == "dsp") {
    auto name = "ptr_" + i.name + ".cap";
    arg->setName(name);

    G.setValuetoMap(name, arg);
    setFvsToMap(i, arg);
    std::advance(arg, 1);
  }
  if (hasmemobj || i.name == "dsp") {
    auto name = i.name + ".mem";
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
  auto& fobjtree = G.funobj_map.at(i.name);
  auto& memobjs = fobjtree->memobjs;
  int count = 0;
  // appending order matters! self should be put on last.
  for (auto& o : memobjs) {
    auto obj = o;
    setMemObj(memarg, obj->fname + ".mem", count++);
  }
  if (fobjtree->hasself) { setMemObj(memarg, "self", count++); }
}

void CodeGenVisitor::setFvsToMap(minst::Function& i, llvm::Value* clsarg) {
  auto& captures = G.cc.getCaptureNames(i.name);

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

llvm::Value* CodeGenVisitor::operator()(minst::Fcall& i) {
  llvm::Value* res;
  bool isclosure = i.ftype == CLOSURE;
  std::vector<llvm::Value*> args;
  auto m = G.variable_map[G.curfunc];
  for (auto& a : i.args) { args.emplace_back(getLlvmVal(a)); }
  auto f = std::get<minst::Function>(std::get<mir::Instructions>(*i.fname));
  if (f.freevariables.size() > 0) {
    // TODO: append memory address made by MakeClosureInst
  }
  if (f.memory_objects.size() > 0) {
    // TODO: append memory address made by memobj collector
  }

  llvm::Value* fun = nullptr;
  switch (i.ftype) {
    case DIRECT: fun = getDirFun(i); break;
    case CLOSURE: fun = getClsFun(i); break;
    case EXTERNAL: fun = getExtFun(i); break;
    default: break;
  }

  if (i.time) {  // if arguments is timed value, call addTask
    res = createAddTaskFn(i, isclosure, isglobal);
  } else {
    res = G.builder->CreateCall(fun, args);
  }
  return res;
}
llvm::Value* CodeGenVisitor::getDirFun(minst::Fcall& i) {
  auto* fun = llvm::cast<llvm::Function>(getLlvmVal(i.fname));
  fun->setLinkage(llvm::GlobalValue::InternalLinkage);
  if (fun == nullptr) { throw std::runtime_error("function could not be referenced"); }
  return fun;
}
llvm::Value* CodeGenVisitor::getClsFun(minst::Fcall& i) { return getDirFun(i); }
llvm::Value* CodeGenVisitor::getExtFun(minst::Fcall& i) {
  auto fun = std::get<minst::Function>(std::get<mir::Instructions>(*i.fname));

  auto it = LLVMBuiltin::ftable.find(fun.name);
  if (it == LLVMBuiltin::ftable.end()) {
    throw std::runtime_error("could not find external function \"" + fun.name + "\"");
  }
  auto f = std::get<minst::Function>(std::get<mir::Instructions>(*i.fname));

  return G.getForeignFunction(fun.name);
}

llvm::Value* CodeGenVisitor::createAddTaskFn(minst::Fcall& i, const bool isclosure,
                                             const bool /*isglobal*/) {
  auto* i8ptrty = G.builder->getInt8PtrTy();
  auto* timeval = getLlvmVal(i.time.value());
  auto* targetfn = G.module->getFunction(getLlvmVal(i.fname)->getName());
  auto* ptrtofn = llvm::ConstantExpr::getBitCast(targetfn, i8ptrty);

  std::vector<llvm::Value*> args = {timeval, ptrtofn};
  for (auto& a : i.args) { args.emplace_back(getLlvmVal(a)); }
  if (i.args.empty()) {
    auto* zero = llvm::ConstantFP::get(G.ctx, llvm::APFloat(0.0));
    args.emplace_back(zero);
  }

  llvm::Value* addtask_fn = nullptr;
  auto globalspace = G.variable_map[G.mainentry->getParent()];
  if (isclosure) {
    llvm::Value* capptr;  // TODO
    // fixme
    if (capptr == nullptr) { capptr = G.tryfindValue("ptr_" + i.name + "_cls"); }
    args.push_back(G.builder->CreateBitCast(capptr, i8ptrty));

    addtask_fn = globalspace->find("addTask_cls")->second;
  } else {
    addtask_fn = globalspace->find("addTask")->second;
  }
  return G.builder->CreateCall(addtask_fn, args);
}

// store the capture address to memory
llvm::Value* CodeGenVisitor::operator()(minst::MakeClosure& i) {
  // auto& capturenames = G.cc.getCaptureNames(i.fname);
  // auto* closuretype = G.getType(G.cc.getCaptureType(i.fname));

  // auto* targetf = G.module->getFunction(i.fname);

  // auto captureptrname = "ptr_" + i.fname + ".cap";
  // // always heap allocation!
  // auto* capture_ptr = createAllocation(true, closuretype, nullptr, captureptrname);
  // auto* fun_ptr =
  //     G.builder->CreateStructGEP(closure_ptr, 0, i.lv_name + "_fun_ptr");
  // G.builder->CreateStore(targetf, fun_ptr);
  // auto* capture_ptr =
  //     G.builder->CreateStructGEP(closure_ptr, 1, i.lv_name +
  //     "_capture_ptr");
  // unsigned int idx = 0;
  // for (auto& cap : capturenames) {
  //   llvm::Value* fv = G.tryfindValue("ptr_" + cap);
  //   if (fv == nullptr) { fv = G.findValue("ptr_" + cap + ".cap"); }
  //   auto* gep = G.builder->CreateStructGEP(capture_ptr, idx, "capture_" + cap);
  //   G.builder->CreateStore(fv, gep);
  //   idx += 1;
  // }
  // G.setValuetoMap(i.lv_name + "_capture_ptr", capture_ptr);
  // G.setValuetoMap(captureptrname, capture_ptr);
  // G.setValuetoMap("ptr_" + closureptrname, closure_ptr);
}
llvm::Value* CodeGenVisitor::operator()(minst::Array& i) {}
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
llvm::Value* CodeGenVisitor::operator()(minst::Field& i) {
  auto* target = getLlvmVal(i.target);
  auto* index = getLlvmVal(i.index);
  auto* tupleptrtype = llvm::PointerType::get(target->getType(), 0);
  return G.builder->CreateInBoundsGEP(target, {index, G.getZero()}, "tupleaccess");
}

void CodeGenVisitor::createIfBody(mir::blockptr& block, llvm::Value* ret_ptr) {
  auto& insts = block->instructions;
  if (ret_ptr == nullptr) {
    for (auto&& ti : insts) { visit(ti); }
  } else {
    if (!std::holds_alternative<minst::Return>(std::get<mir::Instructions>(*insts.back()))) {
      throw std::logic_error("non-void block should have minst::Return for last line");
    }
    for (auto&& iter = std::begin(insts); iter != std::prev(std::end(insts)); iter++) {
      visit(*iter);
    }
    auto& retinst = std::get<minst::Return>(std::get<mir::Instructions>(*insts.back()));
    G.builder->CreateStore(getLlvmVal(retinst.val), ret_ptr);
  }
}

llvm::Value* CodeGenVisitor::operator()(minst::If& i) {
  auto* thisbb = G.builder->GetInsertBlock();
  auto* cond = getLlvmVal(i.cond);
  auto* cmp = G.builder->CreateFCmpOGT(cond, llvm::ConstantFP::get(G.ctx, llvm::APFloat(0.0)));
  auto* endbb = llvm::BasicBlock::Create(G.ctx, i.name + "_end", G.curfunc);

  llvm::Value* retptr;  // TODO
  auto* thenbb = llvm::BasicBlock::Create(G.ctx, i.name + "_then", G.curfunc);
  G.builder->SetInsertPoint(thenbb);
  createIfBody(i.thenblock, retptr);
  G.builder->CreateBr(endbb);
  auto* elsebb = llvm::BasicBlock::Create(G.ctx, i.name + "_else", G.curfunc);
  G.builder->SetInsertPoint(elsebb);
  if (i.elseblock.has_value()) { createIfBody(i.elseblock.value(), retptr); }
  G.builder->CreateBr(endbb);
  G.builder->SetInsertPoint(endbb);
  auto& ifrettype = i.type;
  bool isvoid = std::holds_alternative<types::Void>(ifrettype);
  llvm::Value* res = nullptr;
  if (!isvoid) { res = G.builder->CreateLoad(retptr, i.name); }

  G.builder->SetInsertPoint(thisbb);
  G.builder->CreateCondBr(cmp, thenbb, elsebb);
  G.builder->SetInsertPoint(endbb);
  return res;
}
llvm::Value* CodeGenVisitor::operator()(minst::Return& i) {
  auto* res = getLlvmVal(i.val);
  // store self value
  if (context_hasself) {
    llvm::Value* selfv;  // TODO
    G.builder->CreateStore(res, selfv);
  }
  return G.builder->CreateRet(res);
}
}  // namespace mimium