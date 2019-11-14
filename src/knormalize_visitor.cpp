#include "knormalize_visitor.hpp"

namespace mimium {

KNormalizeVisitor::KNormalizeVisitor(std::shared_ptr<TypeInferVisitor> _typeinfer){
  typeinfer = _typeinfer;
  init();
}

// KNormalizeVisitor::~KNormalizeVisitor(){

// }
std::string KNormalizeVisitor::makeNewName(){
  std::string name = "k" + std::to_string(var_counter);
  var_counter++;
  return name;
}
std::string KNormalizeVisitor::getVarName(){
  auto copiedname = tmpname;
  if(copiedname.size()>0){
    tmpname = "";
    return copiedname;
  }else{
    return makeNewName();
  }
}

void KNormalizeVisitor::init(){
  var_counter=1;
  rootblock.reset();
  rootblock = std::make_shared<MIRblock>("main");
  currentblock = rootblock;
  current_context=nullptr;
  while (!res_stack.empty()) {
    res_stack.pop(); // make stack empty
  }
}
// AST_Ptr KNormalizeVisitor::insertAssign(AST_Ptr ast) {
//   if (ast->getid() == OP) {
//     auto tmpsymbol =
//         std::make_shared<LvarAST>(makeNewName());
//     ast->accept(*this);
    
//     auto assign = std::make_unique<AssignAST>(tmpsymbol, ast);  // recursive!!
//     var_counter++;
//     current_context->appendAST(std::move(assign));
//     return std::make_shared<RvarAST>(ast->);
//   } else {
//     return ast;
//   }
// }

void KNormalizeVisitor::visit(ListAST& ast) {
    auto tempctx = current_context;
  current_context = std::make_shared<ListAST>();
  for (auto& elem : ast.getElements()) {
    elem->accept(*this);
  }
  // //res_stack.push(current_context);
  // if(tempctx!=nullptr){//if top level, keep currentcontext to return
  //   current_context = tempctx;//switch back ctx
  // }
}

void KNormalizeVisitor::visit(OpAST& ast) {
  auto name = getVarName();
  ast.lhs->accept(*this);  // recursively visit
  auto nextlhs = stack_pop_str();
  ast.rhs->accept(*this);
  auto nextrhs = stack_pop_str();
  Instructions newinst = std::make_shared<OpInst>(name,ast.getOpStr(),std::move(nextlhs),std::move(nextrhs));
  currentblock->addInst(newinst);
  res_stack_str.push(name);
}

void KNormalizeVisitor::visit(AssignAST& ast) {
  tmpname = ast.getName()->getVal();
  ast.getBody()->accept(*this);  // result is stored into res_stack
}


void KNormalizeVisitor::visit(NumberAST& ast){
  auto name = getVarName();
  Instructions newinst = std::make_shared<NumberInst>(name,ast.getVal());
  currentblock->addInst(newinst);
  res_stack_str.push(name);
}
void KNormalizeVisitor::visit(LvarAST& ast){
    res_stack_str.push(ast.getVal());
}
void KNormalizeVisitor::visit(RvarAST& ast){
    res_stack_str.push(ast.getVal());
}
void KNormalizeVisitor::visit(LambdaAST& ast){
    auto tmpcontext = currentblock;
    auto name = getVarName();
    auto args = ast.getArgs()->getElements();
    std::deque<std::string> newargs;
    for(auto& arg : args ){
      arg->accept(*this);
      newargs.push_back(stack_pop_str());
    }
    auto newinst = std::make_shared<FunInst>(name,std::move(newargs));
    Instructions res = newinst;
    currentblock->indent_level++;
    currentblock->addInst(res);
    newinst->body->indent_level = currentblock->indent_level;
    currentblock = newinst->body;//move context
    ast.getBody()->accept(*this);
    currentblock = tmpcontext;//switch back context
    currentblock->indent_level--;

}
void KNormalizeVisitor::visit(FcallAST& ast){
  auto newname = getVarName();
  std::deque<std::string> newarg;
  for(auto& arg: ast.getArgs()->getElements()){
    arg->accept(*this);
    newarg.push_back(stack_pop_str());
  }
  Instructions newinst =std::make_shared<FcallInst>(newname,ast.getFname()->getVal(),std::move(newarg));
  currentblock->addInst(newinst);
  res_stack_str.push(newname);
};
void KNormalizeVisitor::visit(ArgumentsAST& ast){
//this won't be used?
}
void KNormalizeVisitor::visit(ArrayAST& ast){
    std::deque<std::string> newelem;
    for(auto& elem: ast.getElements()){
      elem->accept(*this);
      newelem.push_back(stack_pop_str());
    }
    auto newname = getVarName();

    auto newinst =std::make_shared<ArrayInst>(newname,std::move(newelem));
    Instructions res = newinst;
    currentblock->addInst(res);
    res_stack_str.push(newname); 
}
void KNormalizeVisitor::visit(ArrayAccessAST& ast){//access index may be expr
    ast.getIndex()->accept(*this);
    auto newname =getVarName();
    auto newinst = std::make_shared<ArrayAccessInst>(newname,ast.getName()->getVal(),stack_pop_str());        Instructions res = newinst;

    currentblock->addInst(res);
    res_stack_str.push(newname);
}
void KNormalizeVisitor::visit(IfAST& ast){ 
  auto tmpcontext = currentblock;
    ast.getCond()->accept(*this);
    auto newname = getVarName();
    auto newinst =std::make_shared<IfInst>(newname,stack_pop_str());
    Instructions res = newinst;
    currentblock->indent_level++;
    newinst->thenblock->indent_level = currentblock->indent_level;
    newinst->elseblock->indent_level = currentblock->indent_level;
    currentblock->addInst(res);
    currentblock = newinst->thenblock;
    ast.getThen()->accept(*this);
    currentblock = newinst->elseblock;
    ast.getElse()->accept(*this);
    res_stack_str.push(newname);
    currentblock = tmpcontext;
    currentblock->indent_level--;
}
void KNormalizeVisitor::visit(ReturnAST& ast){
  ast.getExpr()->accept(*this);
  auto newname = getVarName();
  Instructions newinst = std::make_shared<ReturnInst>(newname,stack_pop_str());
  currentblock->addInst(newinst);
  res_stack_str.push(newname);
}
void KNormalizeVisitor::visit(ForAST& ast){
//後回し
}
void KNormalizeVisitor::visit(DeclarationAST& ast){
    current_context->appendAST(std::make_unique<DeclarationAST>(ast));
}
void KNormalizeVisitor::visit(TimeAST& ast){
  //ここ考えどこだな　functionにパスするときのなんかもここで処理するかMIRの先で処理するか
  //とりあえずこの先で処理します
    ast.getTime()->accept(*this);
    ast.getExpr()->accept(*this);
    auto newname = getVarName();
    auto newinst =std::make_shared<TimeInst>(newname,stack_pop_str(),stack_pop_str());
    Instructions res = newinst;
    currentblock->addInst(res);
  res_stack_str.push(newname);
}

void KNormalizeVisitor::visit(StructAST& ast){
  //will not be used for now,,,
}
void KNormalizeVisitor::visit(StructAccessAST& ast){
  //will not be used for now,,,

  // ast.getVal()->accept(*this);
  // ast.getKey()->accept(*this);
  // auto newast = std::make_unique<StructAccessAST>(stack_pop_ptr(),stack_pop_ptr());
  // res_stack.push(std::move(newast));
}

std::shared_ptr<MIRblock> KNormalizeVisitor::getResult(){
  return rootblock;
}



}  // namespace mimium