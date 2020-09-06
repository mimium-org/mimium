/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/helper_functions.hpp"
#include "basic/ast.hpp"
#include "basic/mir.hpp"
#include "basic/type.hpp"

#include "compiler/ast_loader.hpp"
#include "compiler/symbolrenamer.hpp"
#include "compiler/type_infer_visitor.hpp"
#include "compiler/mirgenerator.hpp"
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
    mir::blockptr generateMir(AstPtr ast);
    mir::blockptr closureConvert(mir::blockptr mir);
    mir::blockptr collectMemoryObjs(mir::blockptr mir);

    llvm::Module& generateLLVMIr(mir::blockptr mir);
    auto moveLLVMModule(){return llvmgenerator.moveModule();}
 private:
  Driver driver;
  // AlphaConvertVisitor alphavisitor;
  SymbolRenamer symbolrenamer;
  TypeInferer typeinferer;
  MirGenerator mirgenerator;
  std::shared_ptr<ClosureConverter> closureconverter;
  MemoryObjsCollector memobjcollector;
  LLVMGenerator llvmgenerator;
  std::string path;
};

}  // namespace mimium