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
};

}//namespace mimium