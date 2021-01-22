/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/codegen/llvmgenerator.hpp"
#include "compiler/codegen/codegen_visitor.hpp"
#include "compiler/codegen/typeconverter.hpp"

namespace mimium {

LLVMGenerator::LLVMGenerator(llvm::LLVMContext& ctx)
    : ctx(ctx),
      module(std::make_unique<llvm::Module>("no_file_name.mmm", ctx)),
      builder(std::make_unique<llvm::IRBuilder<>>(ctx)),
      mainentry(nullptr),
      currentblock(nullptr),
      curfunc(nullptr),
      typeconverter(std::make_unique<TypeConverter>(*builder, *module)),
      runtime_fun_names(
          {{"mimium_getnow", llvm::FunctionType::get(getDoubleTy(), {geti8PtrTy()}, false)},
           {"access_array_lin_interp",
            llvm::FunctionType::get(
                getDoubleTy(), {llvm::PointerType::get(getDoubleTy(), 0), getDoubleTy()}, false)},
           {"mimium_malloc",
            llvm::FunctionType::get(geti8PtrTy(), {geti8PtrTy(), geti64Ty()}, false)}}) {}
void LLVMGenerator::init(std::string filename) {
  module->setSourceFileName(filename);
  module->setModuleIdentifier(filename);
  module->setTargetTriple(llvm::sys::getDefaultTargetTriple());
}

void LLVMGenerator::setDataLayout(const llvm::DataLayout& dl) { module->setDataLayout(dl); }

void LLVMGenerator::reset(std::string filename) {
  dropAllReferences();
  init(filename);
}

LLVMGenerator::~LLVMGenerator() { dropAllReferences(); }
void LLVMGenerator::dropAllReferences() {
  if (module != nullptr) { module->dropAllReferences(); }
}

llvm::Type* LLVMGenerator::getType(types::Value const& type) {
  return std::visit(*typeconverter, type);
}
llvm::ArrayType* LLVMGenerator::getArrayType(types::Value const& type) {
  assert(rv::holds_alternative<types::Array>(type));
  const auto& atype = rv::get<types::Array>(type);
  return llvm::ArrayType::get(std::visit(*typeconverter, atype.elem_type), atype.size);
}

llvm::Type* LLVMGenerator::getClosureToFunType(types::Value& type) {
  auto aliasty = rv::get<types::Alias>(type);
  auto clsty = rv::get<types::Closure>(aliasty.target);

  auto fty = rv::get<types::Function>(clsty.fun.val);
  fty.arg_types.emplace_back(types::Ref{clsty.captures});
  return (*typeconverter)(fty);
}
void LLVMGenerator::switchToMainFun() {
  setBB(mainentry);
  currentblock = mainentry;
  codegenvisitor->recursivefn_ptr = nullptr;
  curfunc = mainentry->getParent();
}
llvm::Function* LLVMGenerator::getForeignFunction(const std::string& name) {
  auto& [type, targetname] = LLVMBuiltin::ftable.find(name)->second;
  auto ftype = rv::get<types::Function>(type);
  if (name == "delay") { ftype.arg_types.emplace_back(types::Ref{types::delaystruct}); }
  if (!types::isPrimitive(ftype.ret_type)) {
    // for loadwavfile
    ftype.ret_type = types::Ref{ftype.ret_type};
  }
  return getFunction(targetname, getType(ftype));
}
llvm::Function* LLVMGenerator::getRuntimeFunction(const std::string& name) {
  const auto& type = runtime_fun_names.at(name);
  return getFunction(name, type);
}

llvm::Function* LLVMGenerator::getFunction(const std::string& name, llvm::Type* type) {
  auto* funtype = llvm::cast<llvm::FunctionType>(type);
  auto fnc = module->getOrInsertFunction(name, funtype);
  auto* fn = llvm::cast<llvm::Function>(fnc.getCallee());
  fn->setCallingConv(llvm::CallingConv::C);
  return fn;
}

void LLVMGenerator::setBB(llvm::BasicBlock* newblock) { builder->SetInsertPoint(newblock); }
void LLVMGenerator::createMiscDeclarations() {
  // create malloc
  auto* i8 = builder->getInt8Ty();
  auto* i8ptr = builder->getInt8PtrTy();
  auto* i64 = builder->getInt64Ty();
  auto* b = builder->getInt1Ty();
  // create llvm memset
  auto* memsettype = llvm::FunctionType::get(builder->getVoidTy(), {i8ptr, i8, i64, b}, false);
  module->getOrInsertFunction("llvm.memset.p0i8.i64", memsettype).getCallee();
  for (auto [name, type] : runtime_fun_names) {
    module->getOrInsertFunction(name, llvm::cast<llvm::FunctionType>(type));
  }
  module->getOrInsertGlobal("global_runtime", geti8PtrTy());
  auto* gruntimeptr = module->getNamedGlobal("global_runtime");
  gruntimeptr->setLinkage(llvm::GlobalValue::LinkageTypes::PrivateLinkage);
  gruntimeptr->setInitializer(llvm::ConstantPointerNull::get(geti8PtrTy()));
}

// Create mimium_main() function it returns address of closure object for dsp()
// function if it exists.

void LLVMGenerator::createMainFun() {
  auto* fntype = llvm::FunctionType::get(builder->getInt8PtrTy(), {builder->getInt8PtrTy()}, false);
  auto* mainfun =
      llvm::Function::Create(fntype, llvm::Function::ExternalLinkage, "mimium_main", *module);
  mainfun->args().begin()->setName("runtime_ptr");
  mainfun->setCallingConv(llvm::CallingConv::C);
  curfunc = mainfun;
  using Akind = llvm::Attribute;
  std::vector<Akind::AttrKind> attrs = {Akind::NoUnwind, Akind::NoInline, Akind::OptimizeNone};
  llvm::AttributeSet aset;
  for (auto& a : attrs) { aset = aset.addAttribute(ctx, a); }

  mainfun->addAttributes(llvm::AttributeList::FunctionIndex, aset);
  mainentry = llvm::BasicBlock::Create(ctx, "entry", mainfun);
}
void LLVMGenerator::createTaskRegister(bool isclosure = false) {
  std::vector<llvm::Type*> argtypes = {
      builder->getInt8PtrTy(),  // address to runtime instance
      builder->getDoubleTy(),   // time
      builder->getInt8PtrTy(),  // address to function
      builder->getDoubleTy()    // argument(single)
  };
  std::string name = "addTask";
  if (isclosure) {
    argtypes.emplace_back(
        builder->getInt8PtrTy());  // address to closure args(instead of void* type)
    name = "addTask_cls";
  }
  auto* fntype = llvm::FunctionType::get(builder->getVoidTy(), argtypes, false);
  auto addtask = module->getOrInsertFunction(name, fntype);
  auto* addtaskfun = llvm::cast<llvm::Function>(addtask.getCallee());

  addtaskfun->setCallingConv(llvm::CallingConv::C);
  using Akind = llvm::Attribute;
  std::vector<Akind::AttrKind> attrs = {Akind::NoUnwind, Akind::NoInline, Akind::OptimizeNone};
  llvm::AttributeSet aset;
  for (auto& a : attrs) { aset = aset.addAttribute(ctx, a); }
  addtaskfun->addAttributes(llvm::AttributeList::FunctionIndex, aset);
}

void LLVMGenerator::createNewBasicBlock(std::string name, llvm::Function* f) {
  auto* bb = llvm::BasicBlock::Create(ctx, name, f);
  builder->SetInsertPoint(bb);
  currentblock = bb;
}
std::optional<int> LLVMGenerator::getDspFnChannelNumForType(types::Value const& t) {
  // if (std::holds_alternative<types::Float>(t)) { return 1; }
  if (rv::holds_alternative<types::Tuple>(t)) {
    const auto& ttype = rv::get<types::Tuple>(t);
    for (const auto& at : ttype.arg_types) {
      if (!std::holds_alternative<types::Float>(at)) { return std::nullopt; }
    }
    return ttype.arg_types.size();
  }
  if (std::holds_alternative<types::Void>(t)) { return 0; }
  return std::nullopt;
}

void LLVMGenerator::checkDspFunctionType(minst::Function const& i) {
  assert(i.name == "dsp");
  assert(rv::holds_alternative<types::Function>(i.type));
  auto rettype = rv::get<types::Function>(i.type).ret_type;
  auto argtype = rv::get<types::Function>(i.type).arg_types;

  std::optional<int> outchs = std::nullopt;
  std::optional<int> inchs = std::nullopt;

  if (i.args.ret_ptr) {
    auto retptrty = i.args.ret_ptr.value()->type;
    auto ptrty = rv::get<types::Pointer>(retptrty);
    if (auto* ttype = std::get_if<types::rTuple>(&ptrty.val)) {
      outchs = getDspFnChannelNumForType(ptrty.val);
    }
  } else {
    outchs = getDspFnChannelNumForType(rettype);
  }
  switch (i.args.args.size()) {
    case 0: inchs = 0; break;
    case 1: {
      auto inputargidx = i.args.ret_ptr ? 1 : 0;
      inchs = getDspFnChannelNumForType(argtype.at(inputargidx));
    } break;
    default:
      throw std::runtime_error("Number of Arguments for dsp function must be 0 or 1.");
      break;
  }
  if (!inchs) { throw std::runtime_error("Arguments for dsp function must be 1 Tuple of Floats"); }
  if (!outchs) {
    throw std::runtime_error(
        "Return type for dsp function must be either of Void or Tuple of Floats");
  }
  runtime_dspfninfo.in_numchs = inchs.value();
  runtime_dspfninfo.out_numchs = outchs.value();
}

void LLVMGenerator::createRuntimeSetDspFn(llvm::Type* memobjtype) {
  auto* voidptrtype = builder->getInt8PtrTy();
  auto* int32ty = builder->getInt32Ty();
  auto* dspfn = module->getFunction("dsp");
  auto* dspfnaddress = builder->CreateBitCast(dspfn, voidptrtype);
  auto* dspclsaddress = (runtime_dspfninfo.capptr != nullptr)
                            ? builder->CreateBitCast(runtime_dspfninfo.capptr, voidptrtype)
                            : llvm::ConstantPointerNull::get(voidptrtype);
  llvm::Value* dspmemobjaddress = nullptr;
  if (memobjtype != nullptr) {
    auto* dspmemobjptr = codegenvisitor->createAllocation(true, memobjtype, nullptr, "dsp.mem");
    dspmemobjaddress = builder->CreateBitCast(dspmemobjptr, voidptrtype);

    // insert 0 initialization of memobjs
    auto* memsetfn = module->getFunction("llvm.memset.p0i8.i64");
    auto* t = llvm::cast<llvm::PointerType>(dspmemobjptr->getType())->getElementType();
    auto size = module->getDataLayout().getTypeAllocSize(t);
    constexpr int bitsize = 8;
    builder->CreateCall(memsetfn, {dspmemobjaddress, getConstInt(0, bitsize), getConstInt(size),
                                   getConstInt(0, 1)});
  } else {
    dspmemobjaddress = llvm::ConstantPointerNull::get(voidptrtype);
  }
  auto setdsp = module->getOrInsertFunction(
      "setDspParams",
      llvm::FunctionType::get(
          builder->getVoidTy(),
          {voidptrtype, voidptrtype, voidptrtype, voidptrtype, int32ty, int32ty}, false));
  auto* inchs_const = getConstInt(runtime_dspfninfo.in_numchs, 32);
  auto* outchs_const = getConstInt(runtime_dspfninfo.out_numchs, 32);

  builder->CreateCall(setdsp, {getRuntimeInstance(), dspfnaddress, dspclsaddress, dspmemobjaddress,
                               inchs_const, outchs_const});
}

llvm::Value* LLVMGenerator::getRuntimeInstance() {
  auto* var = module->getNamedGlobal("global_runtime");
  assert(var != nullptr);
  return builder->CreateLoad(var, "runtimeptr");
}

void LLVMGenerator::preprocess() {
  createMainFun();
  createMiscDeclarations();
  createTaskRegister(true);   // for non-closure function
  createTaskRegister(false);  // for closure
  setBB(mainentry);
  builder->CreateStore(mainentry->getParent()->args().begin(),
                       module->getNamedGlobal("global_runtime"), false);
}
void LLVMGenerator::visitInstructions(mir::valueptr inst, bool isglobal) {
  codegenvisitor->isglobal = isglobal;
  if (auto* i = std::get_if<mir::Instructions>(inst.get())) {
    codegenvisitor->instance_holder = inst;
    llvm::Value* res = std::visit(*codegenvisitor, *i);
    codegenvisitor->registerLlvmVal(inst, res);
  }
}

void LLVMGenerator::generateCode(mir::blockptr mir, const funobjmap* funobjs) {
  codegenvisitor = std::make_shared<CodeGenVisitor>(*this, funobjs);
  preprocess();
  llvm::Type* memobjtype = nullptr;
  for (auto& inst : mir->instructions) {
    visitInstructions(inst, true);
    if (mir::getName(*inst) == "dsp") {
      auto&& iter = funobjs->find(inst);
      if (iter != funobjs->end()) { memobjtype = getType(iter->second->objtype); }
    }
  }
  if (module->getFunction("dsp") != nullptr) { createRuntimeSetDspFn(memobjtype); }
  // main always return null for now;
  builder->CreateRet(llvm::ConstantPointerNull::get(builder->getInt8PtrTy()));
}

void LLVMGenerator::outputToStream(llvm::raw_ostream& stream) {
  module->print(stream, nullptr, false, true);
}

void LLVMGenerator::dumpvar(llvm::Value* v) {
  v->print(llvm::errs(), true);
  llvm::errs() << "\n";
}
void LLVMGenerator::dumpvar(llvm::Type* v) {
  v->print(llvm::errs(), true);
  llvm::errs() << "\n";
}

}  // namespace mimium