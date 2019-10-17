#include "closureconvert_visitor.hpp"

namespace mimium {
ClosureConvertVisitor::ClosureConvertVisitor(){}
ClosureConvertVisitor::~ClosureConvertVisitor(){}
void ClosureConvertVisitor::visit(OpAST& ast) {}
void ClosureConvertVisitor::visit(ListAST& ast) {}
void ClosureConvertVisitor::visit(NumberAST& ast) {}
void ClosureConvertVisitor::visit(LvarAST& ast) {}
void ClosureConvertVisitor::visit(RvarAST& ast) {}
void ClosureConvertVisitor::visit(AssignAST& ast) {
    
}
void ClosureConvertVisitor::visit(ArgumentsAST& ast) {}
void ClosureConvertVisitor::visit(ArrayAST& ast) {}
void ClosureConvertVisitor::visit(ArrayAccessAST& ast) {}
void ClosureConvertVisitor::visit(FcallAST& ast) {

}
void ClosureConvertVisitor::visit(LambdaAST& ast) {}
void ClosureConvertVisitor::visit(IfAST& ast) {}
void ClosureConvertVisitor::visit(ReturnAST& ast) {}
void ClosureConvertVisitor::visit(ForAST& ast) {}
void ClosureConvertVisitor::visit(DeclarationAST& ast) {}
void ClosureConvertVisitor::visit(TimeAST& ast) {}
}  // namespace mimium