/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
 
#include "compiler/compiler.hpp"
namespace mimium {
Compiler::Compiler() : Compiler(*std::make_shared<llvm::LLVMContext>()) {}
Compiler::Compiler(llvm::LLVMContext& ctx)
    : driver(),
      alphavisitor(),
      typevisitor(),
      recursivechecker(),
      knormvisitor(typevisitor),
      closureconverter(
          std::make_shared<ClosureConverter>(typevisitor.getEnv())),
      memobjcollector(typevisitor.getEnv()),
      llvmgenerator(ctx, typevisitor.getEnv(),*closureconverter,memobjcollector) {}
Compiler::~Compiler() = default;
void Compiler::setFilePath(std::string path) {
  this->path = path;
  llvmgenerator.init(path);
}
void Compiler::setDataLayout(const llvm::DataLayout& dl) {
  llvmgenerator.setDataLayout(dl);
}
void Compiler::recursiveCheck(AstPtr ast) { ast->accept(recursivechecker); }
AstPtr Compiler::loadSource(std::string source) {
  AstPtr ast = driver.parseString(source);
  // auto ast = driver.getMainAst();
  recursiveCheck(ast);
  return ast;
}
AstPtr Compiler::loadSourceFile(std::string filename) {
  AstPtr ast = driver.parseFile(filename);
  recursiveCheck(ast);
  return ast;
}
AstPtr Compiler::alphaConvert(AstPtr ast) {
  ast->accept(alphavisitor);
  return alphavisitor.getResult();
}
TypeEnv& Compiler::typeInfer(AstPtr ast) { return typevisitor.infer(ast); }

std::shared_ptr<MIRblock> Compiler::generateMir(AstPtr ast) {
  ast->accept(knormvisitor);
  return knormvisitor.getResult();
}
std::shared_ptr<MIRblock> Compiler::closureConvert(
    std::shared_ptr<MIRblock> mir) {
  return closureconverter->convert(mir);
}

std::shared_ptr<MIRblock> Compiler::collectMemoryObjs(
    std::shared_ptr<MIRblock> mir) {
  return memobjcollector.process(mir);
}

llvm::Module& Compiler::generateLLVMIr(std::shared_ptr<MIRblock> mir) {
  llvmgenerator.generateCode(mir);
  return llvmgenerator.getModule();
}
}  // namespace mimium