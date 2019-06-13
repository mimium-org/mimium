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

class LLVMGenerator{
    private: 
        llvm::LLVMContext ctx;
        std::unique_ptr<llvm::Function> curfunc;
        std::shared_ptr<llvm::Module> module;
        std::unique_ptr<llvm::IRBuilder<>> builder; 
    public:
        LLVMGenerator();
        LLVMGenerator(llvm::LLVMContext& _cts);

        ~LLVMGenerator();
        std::shared_ptr<llvm::Module> getModule();

        bool generateCode(std::unique_ptr<ListAST> listast,std::string name);

        llvm::Value* generateNumber(AST_Ptr num);
        llvm::Value* generateOpExpr(AST_Ptr expr);
};