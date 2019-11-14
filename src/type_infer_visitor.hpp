#pragma once
#include "ast.hpp"
#include "type.hpp"
// type inference ... assumed to be visited after finished alpha-conversion(each variable has unique name regardless its scope)

namespace mimium{
class TypeInferVisitor : public ASTVisitor{
        public:
        TypeInferVisitor();
        ~TypeInferVisitor()=default;
        void init();

        void visit(OpAST& ast) override;
        void visit(ListAST& ast) override;
        void visit(NumberAST& ast) override;
        void visit(LvarAST& ast) override;
        void visit(RvarAST& ast) override;
        void visit(AssignAST& ast) override;
        void visit(ArgumentsAST& ast) override;
        void visit(ArrayAST& ast) override;
        void visit(ArrayAccessAST& ast) override;
        void visit(FcallAST& ast) override;
        void visit(LambdaAST& ast) override;
        void visit(IfAST& ast) override;
        void visit(ReturnAST& ast) override;
        void visit(ForAST& ast) override;
        void visit(DeclarationAST& ast) override;
        void visit(TimeAST& ast) override;
        void visit(StructAST& ast)override;
        void visit(StructAccessAST& ast)override;
        mValue findVariable(std::string str) override{return 0;}//??
    private:
        types::Value res_stack;
        TypeEnv typeenv;
        bool has_return;
};
}