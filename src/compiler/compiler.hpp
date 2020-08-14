/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/helper_functions.hpp"
#include "basic/ast_new.hpp"
#include "basic/mir.hpp"
#include "basic/type.hpp"

#include "compiler/ast_loader_new.hpp"
#include "compiler/alphaconvert_visitor.hpp"
#include "compiler/recursive_checker.hpp"
#include "compiler/type_infer_visitor.hpp"
#include "compiler/knormalize_visitor.hpp"
#include "compiler/closure_convert.hpp"
#include "compiler/collect_memoryobjs.hpp"
#include "compiler/codegen/llvmgenerator.hpp"

namespace mimium {
// compiler class  that load source code,analyze, and emit llvm IR.
class Compiler {
public: 
    Compiler();
    Compiler(llvm::LLVMContext& ctx);
    ~Compiler();
    AstPtr loadSource(std::string source);
    AstPtr loadSourceFile(std::string filename);
    void setFilePath(std::string path);
    void setDataLayout(const llvm::DataLayout& dl);
    void setDataLayout();

    AstPtr renameSymbols(AstPtr ast);
    TypeEnv& typeInfer(AstPtr ast);
    std::shared_ptr<MIRblock> generateMir(AstPtr ast);
    std::shared_ptr<MIRblock> closureConvert(std::shared_ptr<MIRblock> mir);
    std::shared_ptr<MIRblock> collectMemoryObjs(std::shared_ptr<MIRblock> mir);

    llvm::Module& generateLLVMIr(std::shared_ptr<MIRblock> mir);
    auto moveLLVMModule(){return llvmgenerator.moveModule();}
 private:
    void recursiveCheck(AstPtr ast);
  Driver driver;
  // AlphaConvertVisitor alphavisitor;
  SymbolRenamer symbolrenamer;
  TypeInferer typeinferer;
  RecursiveChecker recursivechecker;
  KNormalizeVisitor knormvisitor;
  std::shared_ptr<ClosureConverter> closureconverter;
  MemoryObjsCollector memobjcollector;
  LLVMGenerator llvmgenerator;
  std::string path;
};

}  // namespace mimium