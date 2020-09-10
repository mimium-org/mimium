/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/compiler.hpp"
namespace mimium {
Compiler::Compiler() : Compiler(*std::make_shared<llvm::LLVMContext>()) {}
Compiler::Compiler(llvm::LLVMContext& ctx)
    : driver(),
      symbolrenamer(std::make_shared<RenameEnvironment>()),
      typeinferer(),
      mirgenerator(typeinferer.getTypeEnv()),
      closureconverter(std::make_shared<ClosureConverter>(typeinferer.getTypeEnv())),
      memobjcollector(typeinferer.getTypeEnv()),
      llvmgenerator(ctx, typeinferer.getTypeEnv(), *closureconverter, memobjcollector) {}
Compiler::~Compiler() = default;
void Compiler::setFilePath(std::string path) {
  this->path = path;
  llvmgenerator.init(path);
}
void Compiler::setDataLayout(const llvm::DataLayout& dl) { llvmgenerator.setDataLayout(dl); }
AstPtr Compiler::loadSource(std::string source) {
  AstPtr ast = driver.parseString(source);
  return ast;
}
AstPtr Compiler::loadSourceFile(std::string filename) {
  AstPtr ast = driver.parseFile(filename);
  return ast;
}
AstPtr Compiler::renameSymbols(AstPtr ast) { return symbolrenamer.rename(*ast); }
TypeEnv& Compiler::typeInfer(AstPtr ast) { return typeinferer.infer(*ast); }

mir::blockptr Compiler::generateMir(AstPtr ast) { return mirgenerator.generate(*ast); }
mir::blockptr Compiler::closureConvert(mir::blockptr mir) { return closureconverter->convert(mir); }

mir::blockptr Compiler::collectMemoryObjs(mir::blockptr mir) {
  return memobjcollector.process(mir);
}

llvm::Module& Compiler::generateLLVMIr(mir::blockptr mir) {
  llvmgenerator.generateCode(mir);
  return llvmgenerator.getModule();
}
}  // namespace mimium