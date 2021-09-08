/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/compiler.hpp"
#include "codegen/llvm_header.hpp"
// #include "compiler/scanner.hpp"

namespace mimium {
Compiler::Compiler()
    : llvmctx(std::make_unique<llvm::LLVMContext>()),
      driver(),
      typeinferer(),
      // mirgenerator(typeinferer.getTypeEnv()),
      closureconverter(std::make_shared<ClosureConverter>()),
      memobjcollector(),
      llvmgenerator(*llvmctx) {}
Compiler::Compiler(std::unique_ptr<llvm::LLVMContext> ctx)
    : llvmctx(std::move(ctx)),
      driver(),
      typeinferer(),
      // mirgenerator(typeinferer.getTypeEnv()),
      closureconverter(std::make_shared<ClosureConverter>()),
      memobjcollector(),
      llvmgenerator(*ctx) {}
Compiler::~Compiler() = default;
void Compiler::setFilePath(std::string path) {
  this->path = path;
  llvmgenerator.init(path);
}
void Compiler::setDataLayout(const llvm::DataLayout& dl) { llvmgenerator.setDataLayout(dl); }

Hast::expr Compiler::loadSource(std::istream& source) { return driver.parse(source); }

Hast::expr Compiler::loadSource(const std::string& source) {
  auto ast = driver.parseString(source);
  return ast;
}
Hast::expr Compiler::loadSourceFile(const std::string& filename) {
  auto ast = driver.parseFile(filename);
  return ast;
}
// Hast::expr Compiler::renameSymbols(AstPtr ast) { return symbolrenamer.rename(*ast); }
LAst::expr  Compiler::lowerAst(Hast::expr const& ast) {
  lowerast::AstLowerer lowerer;
  auto env = std::make_shared<lowerast::AstLowerer::Env>();
  return lowerer.lowerHast(ast,env);
}

TypeEnvH Compiler::typeInfer(LAst::expr& ast) { return typeinferer.infer(ast); }

mir::blockptr Compiler::generateMir(LAst::expr const& ast, TypeEnvH const& tenv) { return mimium::generateMir(ast,tenv); }

mir::blockptr Compiler::closureConvert(mir::blockptr mir) { return closureconverter->convert(mir); }

funobjmap Compiler::collectMemoryObjs(mir::blockptr mir) { return memobjcollector.process(mir); }

llvm::Module& Compiler::generateLLVMIr(mir::blockptr mir, funobjmap const& funobjs) {
  llvmgenerator.generateCode(mir, &funobjs);
  return llvmgenerator.getModule();
}
std::unique_ptr<llvm::LLVMContext> Compiler::moveLLVMCtx() { return std::move(llvmctx); }
std::unique_ptr<llvm::Module> Compiler::moveLLVMModule() { return llvmgenerator.moveModule(); }

void Compiler::dumpLLVMModule(std::ostream& out) {
  std::string str;
  llvm::raw_string_ostream tmpout(str);
  llvmgenerator.getModule().print(tmpout, nullptr);
  out << str;
}

}  // namespace mimium