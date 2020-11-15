/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/codegen/codegen_visitor.hpp"
#include "compiler/codegen/llvmgenerator.hpp"
#include "compiler/codegen/typeconverter.hpp"
#include "compiler/collect_memoryobjs.hpp"

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
CodeGenVisitor::CodeGenVisitor(LLVMGenerator& g, const funobjmap* funobj_map)
    : G(g), isglobal(false), funobj_map(funobj_map) {}

llvm::Value* CodeGenVisitor::visit(mir::valueptr val) {
  instance_holder = val;
  return std::visit(
      overloaded{
          [&](mir::Instructions& inst) { return std::visit(*this, inst); },
          [](mir::Constants& c) -> llvm::Value* { return nullptr; },                 // TODO
          [](std::shared_ptr<mir::Argument> a) -> llvm::Value* { return nullptr; },  // TODO
          [](mir::ExternalSymbol& a) -> llvm::Value* { return nullptr; },            // TODO
          [](mir::Self& a) -> llvm::Value* { return nullptr; },                      // TODO

      },
      *val);
}

void CodeGenVisitor::registerLlvmVal(mir::valueptr mirval, llvm::Value* llvmval) {
  mir_to_llvm.emplace(mirval, llvmval);
}
void CodeGenVisitor::registerLlvmVal(std::shared_ptr<mir::Argument> mirval, llvm::Value* llvmval) {
  mirarg_to_llvm.emplace(mirval, llvmval);
}
void CodeGenVisitor::registerLlvmValforFreeVar(mir::valueptr mirval, llvm::Value* llvmval) {
  mirfv_to_llvm.emplace(mirval, llvmval);
}

llvm::Value* CodeGenVisitor::getLlvmVal(mir::valueptr mirval) {
  // search in order of argument -> local variable -> free variable

  if (auto* arg = std::get_if<std::shared_ptr<mir::Argument>>(mirval.get())) {
    auto iter = mirarg_to_llvm.find(*arg);
    // maybe, value is not contained in mirarg_to_llvm because the value might points argument of
    // outer function.
    if (iter != mirarg_to_llvm.end()) { return iter->second; }
  }

  {
    auto iter = mir_to_llvm.find(mirval);
    // fixme: nested if should be split into subroutine
    if (iter != mir_to_llvm.end()) {
      if (llvm::isa<llvm::Instruction>(iter->second)) {
        if (llvm::cast<llvm::Instruction>(iter->second)->getParent() ==
            G.builder->GetInsertBlock()) {
          return iter->second;
        }
      } else {
        return iter->second;
      }
    }
  }

  {
    auto iter = mirfv_to_llvm.find(mirval);
    if (iter != mirfv_to_llvm.end()) { return iter->second; }
  }
  MMMASSERT(false, "failed to find llvm value for " + mir::getName(*mirval));
  return nullptr;
}

llvm::Value* CodeGenVisitor::createAllocation(bool isglobal, llvm::Type* type,
                                              llvm::Value* arraysize = nullptr,
                                              const llvm::Twine& name = "") {
  if (isglobal) {
    llvm::Type* t = type;
    auto rawname = "ptr_" + name.str() + "_raw";
    auto size = G.module->getDataLayout().getTypeAllocSize(t);
    const int bitsize = 64;
    auto* sizeinst = llvm::ConstantInt::get(G.ctx, llvm::APInt(bitsize, size, false));
    auto* rawres =
        G.builder->CreateCall(G.module->getFunction("mimium_malloc"), {sizeinst}, rawname);
    auto* res = G.builder->CreatePointerCast(rawres, llvm::PointerType::get(t, 0), "ptr_" + name);
    return res;
  }
  return G.builder->CreateAlloca(type, arraysize, "ptr_" + name);
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
  auto* res = createAllocation(isglobal, G.getType(i.type), nullptr, i.name);
  registerLlvmVal(getValPtr(&i), res);
  return res;
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
  mirfv_to_llvm.clear();
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
    // std::vector<types::Value> captype_internal;
    // for (auto& fv : i.freevariables) { captype_internal.emplace_back(mir::getType(*fv)); }
    // argtypes.emplace_back(
    //     types::Alias{i.name + "_cap", types::Ref{types::Tuple{std::move(captype_internal)}}});
  }
  if (hasmemobj || i.name == "dsp") {
    std::vector<types::Value> fvtype_internal;
    for (auto& fv : i.freevariables) { fvtype_internal.emplace_back(mir::getType(*fv)); }
    argtypes.emplace_back(
        types::Alias{i.name + "_fv", types::Ref{types::Tuple{std::move(fvtype_internal)}}});
  }

  return llvm::cast<llvm::FunctionType>((*G.typeconverter)(mmmfntype));
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
    registerLlvmVal(a, arg);
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
  auto& fobjtree = funobj_map->at(i.name);
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
  int count = 0;
  for (auto& fv : i.freevariables) {
    auto* gep = G.builder->CreateStructGEP(clsarg, count++, mir::getName(*fv) + "_ptr");
    // load variable from ptr(closure is reference capture mode)
    auto* val = G.builder->CreateLoad(gep, mir::getName(*fv));
    registerLlvmValforFreeVar(fv, val);
  }
}

llvm::Value* CodeGenVisitor::operator()(minst::Fcall& i) {
  llvm::Value* res = nullptr;
  bool isclosure = i.ftype == CLOSURE;
  std::vector<llvm::Value*> args;
  for (auto& a : i.args) { args.emplace_back(getLlvmVal(a)); }
  // TODO: for closure

  // if (f.memory_objects.size() > 0) {
  //   // TODO: append memory address made by memobj collector
  // }

  llvm::Value* fun = nullptr;
  switch (i.ftype) {
    case DIRECT: fun = getDirFun(i); break;
    case CLOSURE:
      fun = getClsFun(i);
      {
        auto* capptr = G.builder->CreateStructGEP(getLlvmVal(i.fname), 1);
        auto* cap = G.builder->CreateLoad(capptr);
        args.emplace_back(capptr);
      }
      break;
    case EXTERNAL: fun = getExtFun(i); break;
    default: break;
  }

  if (i.time) {  // if arguments is timed value, call addTask
    createAddTaskFn(i, isclosure, isglobal);
  } else {
    if (llvm::cast<llvm::FunctionType>(
            llvm::cast<llvm::PointerType>(fun->getType())->getElementType())
            ->getReturnType()
            ->isVoidTy()) {
      // if return type is void, llvm cannot have return value
      G.builder->CreateCall(fun, args);
    } else {
      res = G.builder->CreateCall(fun, args, i.name);
    }
  }

  return res;
}
llvm::Value* CodeGenVisitor::getDirFun(minst::Fcall& i) {
  auto* fun = llvm::cast<llvm::Function>(getLlvmVal(i.fname));
  if (fun == nullptr) { throw std::runtime_error("function could not be referenced"); }
  return fun;
}
llvm::Value* CodeGenVisitor::getClsFun(minst::Fcall& i) {
  MMMASSERT(
      std::holds_alternative<mir::instruction::MakeClosure>(std::get<mir::Instructions>(*i.fname)),
      "function is not a closure");
  auto* llvar = getLlvmVal(i.fname);
  auto* fptrptr = G.builder->CreateStructGEP(llvar, 0);
  auto* fptr = G.builder->CreateLoad(fptrptr, mir::getName(*i.fname));
  assert(llvm::cast<llvm::FunctionType>(
             llvm::cast<llvm::PointerType>(fptr->getType())->getElementType()) != nullptr);
  return fptr;
}
llvm::Value* CodeGenVisitor::getExtFun(minst::Fcall& i) {
  auto fun = std::get<mir::ExternalSymbol>(*i.fname);
  MMMASSERT(LLVMBuiltin::ftable.count(fun.name) > 0,
            "failed to find external function in llvm conversion.");
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
  assert(types::isClosure(i.type) && "closure type is invalid");
  auto clstype = rv::get<types::Closure>(rv::get<types::Alias>(i.type).target);
  auto* targetf = getLlvmVal(i.fname);

  // // always heap allocation!
  auto* closure_ptr = createAllocation(true, G.getType(i.type), nullptr, i.name);

  auto* fun_ptr = G.builder->CreateStructGEP(closure_ptr, 0, i.name + "_fun_ptr");
  G.builder->CreateStore(targetf, fun_ptr);
  auto* capture_ptr = G.builder->CreateStructGEP(closure_ptr, 1, i.name + "_capture_ptr");
  unsigned int idx = 0;
  for (auto& cap : i.captures) {
    auto* gep = G.builder->CreateStructGEP(capture_ptr, idx++, "capture_" + mir::getName(*cap));
    G.builder->CreateStore(getLlvmVal(cap), gep);
  }
  return closure_ptr;
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