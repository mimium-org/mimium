#include "closureconvert_visitor.hpp"

namespace mimium {
ClosureConvertVisitor::ClosureConvertVisitor(){
    env = std::make_shared<Environment>("root",nullptr);
    closurecount = 0;
}
ClosureConvertVisitor::~ClosureConvertVisitor(){}
void ClosureConvertVisitor::visit(OpAST& ast) {}
void ClosureConvertVisitor::visit(ListAST& ast) {}
void ClosureConvertVisitor::visit(NumberAST& ast) {}
void ClosureConvertVisitor::visit(LvarAST& ast) {}
void ClosureConvertVisitor::visit(RvarAST& ast) {
    if( isFreeVariable(ast.getVal()) ){
        is_closed_function = false;
        auto newast = std::make_shared<StructAccessAST>("Capture",ast.getVal());
    }else{
        is_closed_function = true;
        res_stack.push(std::make_shared<RvarAST>(ast.getVal()));
    }
}
bool ClosureConvertVisitor::isFreeVariable(std::string key){
    bool res=true;
    for(auto& arg : this->args->getElements()){
        if(std::static_pointer_cast<LvarAST>(arg)->getVal() == key){
            res = false; 
            break;
        }
    }
    return res;
}
void ClosureConvertVisitor::visit(AssignAST& ast) {
    ast.getBody()->accept(*this);
    if (ast.getBody()->getid() == LAMBDA){//not smart way,,,
        this->lambdaname_map.emplace(ast.getName()->getVal());
    }else{

    }
}
void ClosureConvertVisitor::visit(ArgumentsAST& ast) {}
void ClosureConvertVisitor::visit(ArrayAST& ast) {}
void ClosureConvertVisitor::visit(ArrayAccessAST& ast) {}
void ClosureConvertVisitor::visit(FcallAST& ast) {
    if( isKnownFunction(ast.getFname()) ){
        res_stack.push(std::make_shared<FcallAST>(ast));
    }else{

    }
}
void ClosureConvertVisitor::visit(LambdaAST& ast) {
    this->args = ast.getArgs();
    
    ast.getBody()->accept(*this);
    if(is_closed_function){
        //case of the function has free variable;
        auto newargs = std::make_shared<ArgumentsAST>(ast.getArgs());//copy construct
        newargs->appendAST(std::make_shared<RvarAST>("Capture") );
        auto newlambda = std::make_shared<LambdaAST>(std::move(newargs),stack_pop());
        auto newast =    std::make_shared<AssignAST>("lambda" + std::to_string(closurecount),std::move(newlambda));
        closurecount++;
        movetoToplevel(std::move(newast));
    }else{
        auto newast = std::make_shared<AssignAST>("")
        movetoTopLevel(std::move(newast));
        addtoKnownFunctions();
    }
}
void ClosureConvertVisitor::movetoToplevel_Closure(AST_Ptr newast){
    
    toplevel.getElements()->push_front(std::move(newast));


}
void ClosureConvertVisitor::visit(IfAST& ast) {}
void ClosureConvertVisitor::visit(ReturnAST& ast) {}
void ClosureConvertVisitor::visit(ForAST& ast) {}
void ClosureConvertVisitor::visit(DeclarationAST& ast) {}
void ClosureConvertVisitor::visit(TimeAST& ast) {}
}  // namespace mimium