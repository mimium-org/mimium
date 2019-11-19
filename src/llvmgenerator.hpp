#pragma once 
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

#include <memory>
#include "ast.hpp"
#include "alphaconvert_visitor.hpp"
#include "knormalize_visitor.hpp"
#include "closure_convert.hpp"
#include "llvm_builtin_functions.hpp"

namespace mimium{
class LLVMBuiltin;
class LLVMGenerator :public std::enable_shared_from_this<LLVMGenerator>{
    private: 
        // std::string filename;
        std::shared_ptr<LLVMBuiltin> builtinfn;

        llvm::Type* getType(types::Value type);
        void preprocess();
        void visitInstructions(Instructions& inst);

    public:
        llvm::LLVMContext ctx;
        std::unique_ptr<llvm::Function> curfunc;
        std::shared_ptr<llvm::Module> module;
        std::unique_ptr<llvm::IRBuilder<>> builder;
        std::unordered_map<std::string, llvm::Value*> namemap;
        std::shared_ptr<llvm::BasicBlock> mainentry;
        std::shared_ptr<llvm::BasicBlock> currentblock;
        explicit LLVMGenerator(std::string _filename);
        // explicit LLVMGenerator(llvm::LLVMContext& _cts,std::string filename);

        ~LLVMGenerator();
        std::shared_ptr<llvm::Module> getModule();
        void setBB(std::shared_ptr<llvm::BasicBlock>  newblock);

        void generateCode(std::shared_ptr<MIRblock> mir);
        void outputToStream(llvm::raw_ostream& ostream);
};
// struct InstructionVisitor{
//     void operator()(NumberInst)
// };
}//namespace mimium