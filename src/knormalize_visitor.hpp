#include "ast.hpp"
namespace mimium{
class KNormalizeVisitor : public ASTVisitor{
        public:
        KNormalizeVisitor();
        ~KNormalizeVisitor();


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

 
        ListAST& getResult();
        private:
        int var_counter;
        std::stack<AST_Ptr> res_stack;
        AST_Ptr stack_pop();//helper
        std::unique_ptr<ListAST> current_context;
        AST_Ptr insert_assign();
        template <class EXPR>
        void KNormalize::exprVisit(EXPR& ast){
            auto res = std::make_shared<EXPR>(ast);
            res_stack.push(std::move(res));  // send result
        } 


};

}//namespace mimium