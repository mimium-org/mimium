#include "compiler/codegen/llvmgenerator.hpp"

namespace mimium {

LLVMGenerator::LLVMGenerator(llvm::LLVMContext& ctx, TypeEnv& typeenv)
    : ctx(ctx),
      module(std::make_unique<llvm::Module>("no_file_name.mmm", ctx)),
      builder(std::make_unique<llvm::IRBuilder<>>(ctx)),
      mainentry(nullptr),
      currentblock(nullptr),
      typeenv(typeenv),
      typeconverter(*builder, *module) {}
void LLVMGenerator::init(std::string filename) {
  codegenvisitor = std::make_shared<CodeGenVisitor>(*this);
  module->setSourceFileName(filename);
  module->setModuleIdentifier(filename);
  module->setTargetTriple(llvm::sys::getDefaultTargetTriple());
}

void LLVMGenerator::setDataLayout(const llvm::DataLayout& dl) {
  module->setDataLayout(dl);
}

void LLVMGenerator::reset(std::string filename) {
  dropAllReferences();
  init(filename);
}

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
  auto aliasty = rv::get<types::Alias>(type);
  auto clsty = rv::get<types::Closure>(aliasty.target);

  auto fty = rv::get<types::Function>(clsty.fun.val);
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
  auto funtype = llvm::cast<llvm::FunctionType>(getType(type));
  auto fnc = module->getOrInsertFunction(name, funtype);
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
      fntype, llvm::Function::ExternalLinkage, "mimium_main", *module);
  mainfun->setCallingConv(llvm::CallingConv::C);
  curfunc = mainfun;
  variable_map.emplace(curfunc, std::make_shared<namemaptype>());
  using Akind = llvm::Attribute;
  std::vector<Akind::AttrKind> attrs = {Akind::NoUnwind, Akind::NoInline,
                                        Akind::OptimizeNone};
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
  std::vector<Akind::AttrKind> attrs = {Akind::NoUnwind, Akind::NoInline,
                                        Akind::OptimizeNone};
  llvm::AttributeSet aset;
  for (auto& a : attrs) {
    aset = aset.addAttribute(ctx, a);
  }
  addtaskfun->addAttributes(llvm::AttributeList::FunctionIndex, aset);
  setValuetoMap(name, addtask.getCallee());
}

void LLVMGenerator::createNewBasicBlock(std::string name, llvm::Function* f) {
  auto* bb = llvm::BasicBlock::Create(ctx, name, f);
  builder->SetInsertPoint(bb);
  currentblock = bb;
}

llvm::Value* LLVMGenerator::getOrCreateFunctionPointer(llvm::Function* f){
  auto name = std::string(f->getName())+"_ptr";
  llvm::Value* funptr = module->getNamedGlobal(name);
  if(funptr==nullptr){
  funptr = module->getOrInsertGlobal(name,llvm::PointerType::get(f->getType(),0));
  builder->CreateStore(f,funptr);
  }
  return funptr;
}

void LLVMGenerator::preprocess() {
  createMainFun();
  createMiscDeclarations();
  createTaskRegister(true);   // for non-closure function
  createTaskRegister(false);  // for closure
  setBB(mainentry);
}
void LLVMGenerator::visitInstructions(Instructions& inst, bool isglobal) {
  codegenvisitor->isglobal = isglobal;
  std::visit(*codegenvisitor, inst);
}

void LLVMGenerator::generateCode(std::shared_ptr<MIRblock> mir) {
  preprocess();
  for (auto& inst : mir->instructions) {
    visitInstructions(inst, true);
  }
  if (mainentry->getTerminator() == nullptr) {  // insert return
    auto dspaddress = variable_map[curfunc]->find("dsp_cls_capture_ptr");
    if (dspaddress != variable_map[curfunc]->end()) {
      auto res = builder->CreateBitCast(dspaddress->second,
                                        builder->getInt8PtrTy(), "res");
      builder->CreateRet(res);
    } else {
      builder->CreateRet(
          llvm::ConstantPointerNull::get(builder->getInt8PtrTy()));
    }
  }
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

void LLVMGenerator::outputToStream(llvm::raw_ostream& stream) {
  module->print(stream, nullptr, false, true);
}

void LLVMGenerator::dumpvars() {
  for (auto& [f, map] : variable_map) {
    llvm::errs() << f->getName() << ":\n";
    for (auto& [key, val] : *map) {
      llvm::errs() << "   " << key << " :  " << val->getName() << "\n";
    }
  }
}
}  // namespace mimium