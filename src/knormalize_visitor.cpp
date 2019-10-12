#include "knormalize_visitor.hpp"

namespace mimium {
AST_Ptr KNormalize::insertAssign(AST_Ptr ast) {
  if (ast->getid() == OP) {
    auto tmpsymbol =
        std::make_shared<SymbolAST>("tmp" + std::to_string(var_counter));
    auto assign = std::make_unique<AssignAST>(tmpsymbol, ast);  // recursive!!
    varcounter++;
    current_context->addAST(std::move(assign));
    return std::move(tmpsymbol);
  } else {
    return std::move(ast);
  }
}

void KNormalize::visit(ListAST& ast) {
    auto tempctx = current_context;
  current_context = std::make_unique<ListAST>();
  for (auto& elem : ast.getlist()) {
    elem->accept(*this);
  }
  res_stack.push(current_context);
  current_context = tempctx;//switch back ctx
}

void KNormalize::visit(OpAST& ast) {
  auto newrhs = insertAssign(ast.rhs);
  auto newlhs = insertAssign(ast.lhs);
  newrhs->accept(*this);
  newlhs->accept(*this);  // recursively visit
  auto res = std::make_unique<OpAST>(ast.getOp(), stack_pop(), stack_pop());
  res_stack.push(res);
}

void KNormalize::visit(AssignAST& ast) {
  ast.getBody()->accept(*this);  // result is stored into tmp_ptr;
  auto newast = std::make_unique<AssignAST>(ast.getname(), stack_pop());  // copy
  current_context->addAST(std::move(newast));
}


void KNormalize::visit<NumberAST>(NumberAST& ast){
    exprVisit(ast);
}
void KNormalize::visit<SymbolAST>(SymbolAST& ast){
    exprVisit(ast);
}
void KNormalize::visit<LambdaAST>(LambdaAST& ast){
    ast.getBody()->accept(*this);  // result is stored into tmp_ptr;
    auto newbody = stack_pop();
    auto newast = std::make_unique<LambdaAST>(ast.getname(), ast.getArgs(),std::move(newbody));  res_stack.push(std::move(newast));
}
void KNormalize::visit(FcallAST& ast){
    exprVisit(ast);
};
void KNormalize::visit(ArgumentsAST& ast){
//this won't be used?
}
void KNormalize::visit(ArrayAST& ast){
    exprVisit(ast);
}
void KNormalize::visit(ArrayAccessAST& ast){//access index may be expr
    ast.getIndex()->accept(*this);
    auto newast = std::make_unique<ArrayAccessAST>(ast.getName(),stack_pop());
    res_stack.push(std::move(newast));
}
void KNormalize::visit(IfAST& ast){
    ast.getElse()->accept(*this);
    ast.getThen()->accept(*this);
    auto newast = std::make_unique<IfAST>(ast.getCond(),stack_pop(),stack_pop());
    currentcontext.addAST(std::move(newast));
}
void KNormalize::visit(ReturnAST& ast){
    ast.getExpr()->accept(*this);
    auto newast  = std::make_unique<ReturnAST>(stack_pop());
    currentcontext.addAST(std::move(newast));
}
void KNormalize::visit(ForAST& ast){
    ast.getExpression()->accept(*this);
    auto newast = std::make_unique<ForAST>(ast.getVar(),ast.getIterator(),stack_pop());
    currentcontext.addAST(std::move(newast));
}
void KNormalize::visit(DeclarationAST& ast){
    currentcontext.addAST(std::make_unique<DeclarationAST>(ast));
}
void KNormalize::visit(TimeAST& ast){
    ast.getTime()->accept(*this);
    ast.getExpr()->accept(*this);
    auto newast = std::make_unique<TimeAST>(stack_pop(),stack_pop());
    res_stack.push(std::move(newast));
}

AST_Ptr stack_pop() {
  auto res = res_stack.top();
  res_stack.pop();
  return std::move(res);
}

}  // namespace mimium