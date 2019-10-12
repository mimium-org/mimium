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
#include "closureconvert_visitor.hpp"
namespace mimium{
class LLVMVisitor:public ASTVisitor,public std::enable_shared_from_this<LLVMVisitor> {
    private: 
        llvm::LLVMContext ctx;
        std::unique_ptr<llvm::Function> curfunc;
        std::shared_ptr<llvm::Module> module;
        std::unique_ptr<llvm::IRBuilder<>> builder;
        AlphaConvertVisitor alphavisitor;
        KNormalizeVisitor knormvisitor;
        ClosureFlattenVisitor closurevisitor;
    public:
        LLVMGenerator();
        LLVMGenerator(llvm::LLVMContext& _cts);

        ~LLVMGenerator();
        std::shared_ptr<llvm::Module> getModule();

        bool generateCode(ListAST& listast, std::string name);

        void visit(OpAST& ast);
        void visit(ListAST& ast);
        void visit(NumberAST& ast);
        void visit(SymbolAST& ast);
        void visit(AssignAST& ast);
        void visit(ArgumentsAST& ast);
        void visit(ArrayAST& ast);
        void visit(ArrayAccessAST& ast);
        void visit(FcallAST& ast);
        void visit(LambdaAST& ast);
        void visit(IfAST& ast);
        void visit(ReturnAST& ast);
        void visit(ForAST& ast);
        void visit(DeclarationAST& ast);
        void visit(TimeAST& ast);
};
}//namespace mimium