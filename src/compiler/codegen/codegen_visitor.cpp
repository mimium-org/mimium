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
    : G(g),
      isglobal(false),
      funobj_map(funobj_map),
      context_hasself(false),
      recursivefn_ptr(nullptr) {}

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
llvm::Value* CodeGenVisitor::getConstant(const mir::Constants& val) {
  return std::visit(overloaded{
                        [&](int v) { return (llvm::Value*)G.getConstInt(v); },
                        [&](double v) { return (llvm::Value*)G.getConstDouble(v); },
                        // todo
                        [](const std::string& v) { return (llvm::Value*)nullptr; },
                    },
                    val);
}

llvm::Value* CodeGenVisitor::getLlvmVal(mir::valueptr mirval) {
  auto* res = std::visit(
      overloaded{[&](mir::Constants& v) { return getConstant(v); },
                 [&](mir::ExternalSymbol& v) {
                   assert(false && "currently should be unreachable");
                   return (llvm::Value*)nullptr;
                 },
                 [&](std::shared_ptr<mir::Argument> v) {
                   auto iter = mirarg_to_llvm.find(v);
                   return (iter != mirarg_to_llvm.end()) ? iter->second : nullptr;
                 },
                 [&](mir::Instructions& v) {
                   auto iter = mir_to_llvm.find(mirval);
                   if (iter != mir_to_llvm.end()) {
                     const bool isinst = llvm::isa<llvm::Instruction>(iter->second);
                     const llvm::Instruction* inst = nullptr;
                     if (isinst) { inst = llvm::cast<llvm::Instruction>(iter->second); }
                     auto* parentfn = G.builder->GetInsertBlock()->getParent();
                     bool is_on_samefunc = isinst && inst->getParent()->getParent() == parentfn;
                     if (is_on_samefunc || !isinst) { return iter->second; }
                   }
                   auto iterfv = mirfv_to_llvm.find(mirval);
                   if (iterfv != mirfv_to_llvm.end()) { return iterfv->second; }
                   return (llvm::Value*)nullptr;
                 },
                 [&](mir::Self v) {
                   auto iter = fun_to_selfval.find(v.fn);
                   if (iter != fun_to_selfval.end()) { return iter->second; }
                   return (llvm::Value*)nullptr;
                 }},
      *mirval);

  assert(res != nullptr && "failed to find llvm value from mir valueptr");
  return res;
}

llvm::Value* CodeGenVisitor::createAllocation(bool isglobal, llvm::Type* type,
                                              llvm::Value* array_size = nullptr,
                                              const llvm::Twine& name = "") {
  if (isglobal) {
    llvm::Type* t = type;
    auto rawname = "ptr_" + name.str() + "_raw";
    auto size = G.module->getDataLayout().getTypeAllocSize(t);
    const int bitsize = 64;
    auto* sizeinst = llvm::ConstantInt::get(G.ctx, llvm::APInt(bitsize, size, false));
    auto* rawres = G.builder->CreateCall(G.module->getFunction("mimium_malloc"),
                                         {G.getRuntimeInstance(), sizeinst}, rawname);
    auto* res = G.builder->CreatePointerCast(rawres, llvm::PointerType::get(t, 0), "ptr_" + name);
    return res;
  }
  return G.builder->CreateAlloca(type, array_size, "ptr_" + name);
};

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
  gvalue->setName(i.name);
  auto* bitcast = G.builder->CreateBitCast(gvalue, i8ptrty, i.name + "_ptr");
  return bitcast;
}
llvm::Value* CodeGenVisitor::operator()(minst::Allocate& i) {
  auto* res = createAllocation(isglobal, G.getType(i.type), nullptr, i.name);
  registerLlvmVal(getValPtr(&i), res);
  return res;
}
llvm::Value* CodeGenVisitor::operator()(minst::Ref& i) {
  // temporarily unused
  assert(false);
  return nullptr;
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
  memobj_to_llvm.clear();
  bool hascapture = !i.freevariables.empty();

  auto fobjtree = funobj_map->find(getValPtr(&i));
  bool hasmemobj = false;
  if (fobjtree != funobj_map->end()) {
    context_hasself = fobjtree->second->hasself;
    hasmemobj = !fobjtree->second->memobjs.empty() || context_hasself;
  }
  bool isdsp = i.name == "dsp";
  if (isdsp) { G.checkDspFunctionType(i); }
  auto* ft = !isdsp ? createFunctionType(i) : createDspFnType(i, hascapture, hasmemobj);
  auto* f = createFunction(ft, i);
  recursivefn_ptr = &i;
  G.curfunc = f;
  G.createNewBasicBlock("entry", f);
  addArgstoMap(f, i, hascapture, hasmemobj);

  for (auto& cinsts : i.body->instructions) { G.visitInstructions(cinsts, false); }

  if (G.builder->GetInsertBlock()->getTerminator() == nullptr && ft->getReturnType()->isVoidTy()) {
    G.builder->CreateRetVoid();
  }
  G.switchToMainFun();
  return f;
}
llvm::FunctionType* CodeGenVisitor::createDspFnType(minst::Function& i, bool hascapture,
                                                    bool hasmemobj) {
  // arguments of dsp function should be always (time, cls_ptr, memobj_ptr) regardless of existences
  // of capture &memobj.
  auto dummytype = types::Ref{types::Void{}};
  auto mmmfntype = rv::get<types::Function>(i.type);
  auto& argtypes = mmmfntype.arg_types;
  auto insertpoint = std::next(argtypes.begin());
  if (!std::holds_alternative<types::Void>(mmmfntype.ret_type)) {
    insertpoint = std::next(insertpoint);
  }
  if (i.args.args.empty()) { argtypes.insert(insertpoint, dummytype); }
  if (!hascapture && !hasmemobj) {
    argtypes.emplace_back(dummytype);
    argtypes.emplace_back(dummytype);
  }
  if (!hasmemobj && hascapture) { argtypes.emplace_back(dummytype); }
  if (hasmemobj && !hascapture) { argtypes.insert(std::prev(argtypes.end()), dummytype); }

  return llvm::cast<llvm::FunctionType>((*G.typeconverter)(mmmfntype));
}

llvm::FunctionType* CodeGenVisitor::createFunctionType(minst::Function& i) {
  auto mmmfntype = rv::get<types::Function>(i.type);
  auto& argtypes = mmmfntype.arg_types;

  // convert function type to function pointer type for high order function
  for (auto& atype : argtypes) {
    if (std::holds_alternative<types::rFunction>(atype)) { atype = types::Pointer{atype}; }
  }
  return llvm::cast<llvm::FunctionType>((*G.typeconverter)(mmmfntype));
}

llvm::Function* CodeGenVisitor::createFunction(llvm::FunctionType* type, minst::Function& i) {
  auto link = llvm::Function::ExternalLinkage;
  auto* f = llvm::Function::Create(type, link, i.name, *G.module);
  return f;
}
void CodeGenVisitor::addArgstoMap(llvm::Function* f, minst::Function& i, bool hascapture,
                                  bool hasmemobj) {
  const bool isdsp = i.name == "dsp";
  // arguments are [actual arguments], capture , memobjs
  auto* arg = std::begin(f->args());
  if (auto a = i.args.ret_ptr) {
    arg->setName(a.value()->name);
    registerLlvmVal(a.value(), arg);
    std::advance(arg, 1);
  }
  for (auto& a : i.args.args) {
    arg->setName(a->name);
    registerLlvmVal(a, arg);
    std::advance(arg, 1);
  }
  if (i.args.args.empty() && isdsp) {
    arg->setName("input");
    std::advance(arg, 1);
  }
  if (hascapture) {
    auto name = "ptr_" + i.name + ".cap";
    arg->setName(name);
    setFvsToMap(i, arg);
    std::advance(arg, 1);
  } else if (isdsp) {
    std::advance(arg, 1);
  }
  if (hasmemobj) {
    auto name = i.name + ".mem";
    arg->setName(name);
    setMemObjsToMap(getValPtr(&i), arg);
  }
}

void CodeGenVisitor::setMemObjsToMap(mir::valueptr fun, llvm::Value* memarg) {
  auto fobjtree = funobj_map->at(fun);
  auto& memobjs = fobjtree->memobjs;
  int count = 0;
  // appending order matters! self should be put on last.
  for (auto& o : memobjs) {
    auto* gep = G.builder->CreateStructGEP(memarg, count++, mir::getName(*o->fname) + ".mem");
    memobj_to_llvm.emplace(o->fname, gep);
  }
  if (fobjtree->hasself) {
    auto* gep = G.builder->CreateStructGEP(memarg, count++, "ptr_self");
    fun_to_selfptr.emplace(fun, gep);
    auto* selfval = G.builder->CreateLoad(gep, "self");
    fun_to_selfval.emplace(fun, selfval);
  }
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
  const bool isclosure = i.ftype == CLOSURE;
  bool isrecursive = false;
  const bool isdsp = i.name == "dsp";
  mir::valueptr mmmfn = i.fname;
  if (auto* fn_i = std::get_if<mir::Instructions>(i.fname.get())) {
    // recursive function detection with pointer comparison
    if (auto* fnptrraw = std::get_if<minst::Function>(fn_i)) {
      isrecursive = fnptrraw == this->recursivefn_ptr;
    }
    if (auto* clsptr = std::get_if<minst::MakeClosure>(fn_i)) {
      auto* fnptr_cls = &mir::getInstRef<minst::Function>(clsptr->fname);
      isrecursive = fnptr_cls == this->recursivefn_ptr;
      mmmfn = clsptr->fname;
    }
  }
  auto fobjtree_iter = funobj_map->find(mmmfn);
  const bool hasmemobj = fobjtree_iter != funobj_map->end();

  auto* fun = isrecursive ? G.curfunc : getFunForFcall(i);
  // prepare arguments
  std::vector<llvm::Value*> args = {};
  if (mir::getName(*i.fname) == "mimium_getnow") { args.push_back(G.getRuntimeInstance()); }
  if (i.time.has_value()) {
    auto* timeval = getLlvmVal(i.time.value());
    llvm::Value* ptrtofn = G.builder->CreateBitCast(fun, G.geti8PtrTy(), fun->getName() + "_i8");
    args = {G.getRuntimeInstance(), timeval, ptrtofn};
    if (i.args.empty()) { args.emplace_back(llvm::ConstantFP::get(G.ctx, llvm::APFloat(0.0))); }
    if (i.args.size() > 1) {
      throw std::runtime_error(
          "currently function call with @ operator can accept only one argument with float type");
    }
    // addTask should be declared in preprocess;
    fun = G.module->getFunction(isclosure ? "addTask_cls" : "addTask");
  }

  std::transform(i.args.begin(), i.args.end(), std::back_inserter(args),
                 [&](mir::valueptr v) { return getLlvmVal(v); });
  if (isclosure) {
    auto* capptr = isrecursive
                       ? std::prev(G.curfunc->arg_end(), (hasmemobj) ? 2 : 1)
                       : G.builder->CreateStructGEP(getLlvmVal(i.fname), 1, i.name + ".cap");
    if (i.time.has_value()) {
      capptr = G.builder->CreateBitCast(capptr, G.geti8PtrTy(), i.name + ".capi8");
    }
    args.emplace_back(capptr);
  }

  if (hasmemobj) {
    auto res = memobj_to_llvm.find(fobjtree_iter->second->fname);
    if (res != memobj_to_llvm.end()) { args.emplace_back(res->second); }
  }
  auto* funtype_raw = fun->getType();
  if (funtype_raw->isPointerTy()) {
    funtype_raw = llvm::cast<llvm::PointerType>(funtype_raw)->getPointerElementType();
  }
  assert(funtype_raw->isFunctionTy());
  auto* ft = llvm::cast<llvm::FunctionType>(funtype_raw);
  // if return type is void, llvm cannot have return value and name
  if (ft->getReturnType()->isVoidTy()) {
    G.builder->CreateCall(ft, fun, args);
    return nullptr;
  }
  return G.builder->CreateCall(ft, fun, args, i.name);
}
llvm::Value* CodeGenVisitor::getFunForFcall(minst::Fcall const& i) {
  switch (i.ftype) {
    case DIRECT: return getDirFun(i);
    case CLOSURE: return getClsFun(i);
    case EXTERNAL: return getExtFun(i);
  }
  assert(false);
  return nullptr;
}

llvm::Value* CodeGenVisitor::getDirFun(minst::Fcall const& i) { return getLlvmVal(i.fname); }
llvm::Value* CodeGenVisitor::getClsFun(minst::Fcall const& i) {
  return std::visit(overloaded{[&](mir::Instructions& f) -> llvm::Value* {
                                 assert(std::holds_alternative<mir::instruction::MakeClosure>(f));
                                 auto& cls = std::get<mir::instruction::MakeClosure>(f);
                                 auto& realf = cls.fname;
                                 return getLlvmVal(realf);
                               },
                               [&](std::shared_ptr<mir::Argument> f) -> llvm::Value* {
                                 return getLlvmVal(i.fname);
                               },
                               [](auto& /*v*/) {
                                 assert(false);
                                 return static_cast<llvm::Value*>(nullptr);
                               }},
                    *i.fname);
}
llvm::Value* CodeGenVisitor::getExtFun(minst::Fcall const& i) {
  auto fun = std::get<mir::ExternalSymbol>(*i.fname);
  if (LLVMBuiltin::ftable.count(fun.name) > 0) { return G.getForeignFunction(fun.name); }
  if (G.runtime_fun_names.count(fun.name) > 0) { return G.getRuntimeFunction(fun.name); }
  assert(false);
  return nullptr;
}

// store the capture address to memory
llvm::Value* CodeGenVisitor::operator()(minst::MakeClosure& i) {
  // auto& capturenames = G.cc.getCaptureNames(i.fname);
  assert(types::isClosure(i.type) && "closure type is invalid");
  auto* targetf = getLlvmVal(i.fname);
  const bool isdsp = targetf->getName() == "dsp";
  auto* closuretype = G.getType(i.type);
  // // always heap allocation!
  auto* closure_ptr = createAllocation(true, closuretype, nullptr, i.name);
  if (!isdsp) {
    auto* fun_ptr = G.builder->CreateStructGEP(closure_ptr, 0, i.name + "_fun_ptr");
    G.builder->CreateStore(targetf, fun_ptr);
  }
  auto* capture_ptr = G.builder->CreateStructGEP(closure_ptr, 1, i.name + "_capture_ptr");
  unsigned int idx = 0;
  for (auto& cap : i.captures) {
    auto* gep = G.builder->CreateStructGEP(capture_ptr, idx++, "capture_" + mir::getName(*cap));
    auto* gepelemty = llvm::cast<llvm::PointerType>(gep->getType())->getContainedType(0);
    auto* capval = getLlvmVal(cap);
    if (gepelemty != capval->getType()) { capval = G.builder->CreateBitCast(capval, gepelemty); }
    G.builder->CreateStore(capval, gep);
  }

  if (isdsp) { G.runtime_dspfninfo.capptr = capture_ptr; }
  return closure_ptr;
}
llvm::Value* CodeGenVisitor::operator()(minst::Array& i) {
  auto* atype = G.getArrayType(i.type);
  auto* gvalue = llvm::cast<llvm::GlobalVariable>(G.module->getOrInsertGlobal(i.name, atype));

  std::vector<llvm::Constant*> values = {};
  std::transform(i.args.cbegin(), i.args.cend(), std::back_inserter(values),
                 [&](mir::valueptr v) { return llvm::cast<llvm::Constant>(getLlvmVal(v)); });
  auto* constantarray = llvm::ConstantArray::get(atype, values);
  gvalue->setInitializer(constantarray);
  auto* first_element = G.builder->CreateInBoundsGEP(gvalue, {G.getZero()}, i.name + "firstelem");
  auto* ptrval = G.builder->CreateBitCast(
      first_element, llvm::PointerType::get(atype->getElementType(), 0), i.name + "_ptr");
  return gvalue;
}
llvm::Value* CodeGenVisitor::operator()(minst::ArrayAccess& i) {
  auto* target = getLlvmVal(i.target);
  auto* index = getLlvmVal(i.index);
  auto* arraccessfun = G.module->getFunction("access_array_lin_interp");
  auto* dptrty = arraccessfun->getArg(0)->getType();
  if (target->getType() != dptrty) { target = G.builder->CreateBitCast(target, dptrty); }
  return G.builder->CreateCall(arraccessfun, {target, index}, "arrayaccess");
}
llvm::Value* CodeGenVisitor::operator()(minst::Field& i) {
  auto* target = getLlvmVal(i.target);
  int index = 0;
  // todo:refactor
  if (auto* constant = std::get_if<mir::Constants>(i.index.get())) {
    index =
        std::visit(overloaded{[](std::string& str) {
                                throw std::runtime_error("index of field access must be a number");
                                return 0;
                              },
                              [](const auto& v) { return (int)v; }},
                   *constant);
  } else {
    throw std::runtime_error("index of field access must be a Constant.");
  }
  return G.builder->CreateStructGEP(target, index, "tupleaccess");
}

llvm::Value* CodeGenVisitor::createIfBody(mir::blockptr& block) {
  auto& insts = block->instructions;
  if (!std::holds_alternative<minst::Return>(std::get<mir::Instructions>(*insts.back()))) {
    throw std::logic_error("non-void block should have minst::Return for last line");
  }
  for (auto&& iter = insts.cbegin(); iter != std::prev(insts.cend()); ++iter) {
    G.visitInstructions(*iter, false);
  }
  auto retinst = mir::getInstRef<minst::Return>(insts.back());
  return getLlvmVal(retinst.val);
}

llvm::Value* CodeGenVisitor::operator()(minst::If& i) {
  auto* thisbb = G.builder->GetInsertBlock();
  auto* cond = getLlvmVal(i.cond);
  auto* cmp = G.builder->CreateFCmpOGT(cond, llvm::ConstantFP::get(G.ctx, llvm::APFloat(0.0)));
  auto* endbb = llvm::BasicBlock::Create(G.ctx, i.name + "_end", G.curfunc);

  auto* thenbb = llvm::BasicBlock::Create(G.ctx, i.name + "_then", G.curfunc, endbb);
  G.builder->SetInsertPoint(thenbb);
  auto* thenret = createIfBody(i.thenblock);
  auto* thenbb_last = G.builder->GetInsertBlock();

  G.builder->CreateBr(endbb);
  auto* elsebb = llvm::BasicBlock::Create(G.ctx, i.name + "_else", G.curfunc, endbb);
  G.builder->SetInsertPoint(elsebb);
  llvm::Value* elseret = nullptr;
  if (i.elseblock.has_value()) { elseret = createIfBody(i.elseblock.value()); }

  auto* elsebb_last = G.builder->GetInsertBlock();

  G.builder->CreateBr(endbb);
  G.builder->SetInsertPoint(endbb);
  auto& ifrettype = i.type;
  bool isvoid = std::holds_alternative<types::Void>(ifrettype);
  llvm::Value* res = nullptr;
  if (!isvoid) {
    auto* phinode = G.builder->CreatePHI(G.getType(i.type), 2);
    phinode->addIncoming(thenret, thenbb_last);
    phinode->addIncoming(elseret, elsebb_last);

    res = phinode;
  }
  G.builder->SetInsertPoint(thisbb);
  G.builder->CreateCondBr(cmp, thenbb, elsebb);
  G.builder->SetInsertPoint(endbb);
  return res;
}
llvm::Value* CodeGenVisitor::operator()(minst::Return& i) {
  auto* res = getLlvmVal(i.val);
  // store self value
  if (context_hasself) {
    assert(fun_to_selfptr.count(i.parent->parent.value()) > 0);
    auto* selfptr = fun_to_selfptr.at(i.parent->parent.value());
    G.builder->CreateStore(res, selfptr);
  }
  return G.builder->CreateRet(res);
}
}  // namespace mimium