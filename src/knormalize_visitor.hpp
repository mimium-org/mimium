#pragma once
#include "ast.hpp"
#include <stack>
namespace mimium{
class KNormalizeVisitor : public ASTVisitor{
        public:
        KNormalizeVisitor();
        ~KNormalizeVisitor()=default;
        void init();

        void visit(OpAST& ast) override;
        void visit(ListAST& ast) override;
        void visit(NumberAST& ast) override;
        void visit(SymbolAST& ast) override;
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

        mValue findVariable(std::string str) override{return 0;}//??

        std::shared_ptr<ListAST> getResult();
        private:
        int var_counter;
        std::stack<AST_Ptr> res_stack;
        AST_Ptr stack_pop();//helper
        std::shared_ptr<ListAST> current_context;
        AST_Ptr insertAssign(AST_Ptr ast);
        template <class EXPR>
        void exprVisit(EXPR& ast){
            auto res = std::make_shared<EXPR>(ast);
            res_stack.push(std::move(res));  // send result
        } 


};

}//namespace mimium