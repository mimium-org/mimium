#pragma once
#include <cmath>
#include "ast.hpp"
#include "helper_functions.hpp"
#include "interpreter.hpp"

namespace mimium{

    class InterpreterVisitor: public ASTVisitor, public std::enable_shared_from_this<InterpreterVisitor>{
        public:
        explicit InterpreterVisitor(Interpreter* _itp):itp(_itp){
        
        }

        mValue visit(OpAST& ast);
        mValue visit(ListAST& ast);
        mValue visit(NumberAST& ast);
        mValue visit(SymbolAST& ast);
        mValue visit(AssignAST& ast);
        mValue visit(AbstractListAST& ast);
        mValue visit(ArrayAST& ast);
        mValue visit(ArrayAccessAST& ast);
        mValue visit(FcallAST& ast);
        mValue visit(LambdaAST& ast);
        mValue visit(IfAST& ast);
        mValue visit(ReturnAST& ast);
        mValue visit(ForAST& ast);
        mValue visit(DeclarationAST& ast);
        mValue visit(TimeAST& ast);
        private:
        std::shared_ptr<Interpreter> itp;
    };
}