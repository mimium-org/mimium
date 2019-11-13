#include "type_infer_visitor.cpp"

namespace mimium{

void TypeInferVisitor::visit(LvarAST& ast) {
    //do nothing
}


void TypeInferVisitor::visit(RvarAST& ast) {
    auto newname = std::get<std::string>(env->findVariable(ast.getVal()));
    auto newast = std::make_unique<RvarAST>(newname);
    res_stack.push(std::move(newast));
}
void TypeInferVisitor::visit(OpAST& ast) {
    ast.rhs->accept(*this);
    ast.lhs->accept(*this);
    auto newast = std::make_unique<OpAST>(ast.op,stack_pop_ptr(),stack_pop_ptr());
    res_stack.push(std::move(newast));
}
void TypeInferVisitor::visit(ListAST& ast) {
    listastvisit(ast);
}
void TypeInferVisitor::visit(NumberAST& ast) {
    res_stack = types::Float();
}
void TypeInferVisitor::visit(AssignAST& ast) {

    ast.getBody()->accept(*this);

    typeenv.env[ast.getName()] = res_stack;
}
void TypeInferVisitor::visit(ArgumentsAST& ast) {
    listastvisit(ast);
   
}
void TypeInferVisitor::visit(ArrayAST& ast) {
    auto tmpres = res_stack;
    for(const auto& v : ast.getElements()){
        v->accept(*this);
        if(tmpres.index()!=res_stack.index()){
            throw std::runtime_error("array contains different types.");
        }
        tmpres = res_stack;
    }
    res_stack = type::Array(tmpres);
}
void TypeInferVisitor::visit(ArrayAccessAST& ast) {
    auto it = typeenv.env.find(ast.getName());
    type::Value res;
    if(it==typeenv.env.end()){
        throw std::runtime_error("could not find accessed array");
    }else{
        type::Array* arr = std::get_if<type::Array>(*it);
        if(arr){
            res = (*arr).getElemType();
        }else{
            throw std::runtime_error("accessed value was not an array");
        }
    }
    res_stack = res;
}
void TypeInferVisitor::visit(FcallAST& ast) {

    type::Value v = typeenv.env.find(ast.getFname());
    type::Function* fn = std::get_if<type::Function>(v);
    type::Value res;
    if(fn){
        auto fnargtypes = fn->getArgTypes();
        auto args = ast.getArgs();
    
        std::vector<type::Value> argtypes;
        bool checkflag = false;
        for(int i = 0; i<argtypes.size();i++){
            argtypes[i]->accept(*this);
            checkflag |=  res_stack.index() != fnargtypes[i].index();
        }
        if(checkflag){
            throw std::runtime_error("argument types were invalid");
        }
        res = fn->getReturnType();
    }else{
        throw std::runtime_error("called function was not a function type");
    }

    res_stack = res;
}
void TypeInferVisitor::visit(LambdaAST& ast) {
    std::vector<Values> argtypes;
    for(const auto& a : ast.getArgs()->getElements()){
        a->accept(*this);
        argtypes.push_back(res_stack);
    }
    auto newargs = stack_pop_ptr();
    ast.getBody()->accept(*this);
    auto newbody = stack_pop_ptr();
    auto newast = std::make_unique<LambdaAST>(std::move(newargs),std::move(newbody));
    res_stack.push(std::move(newast));
    env = env->getParent();
}
void TypeInferVisitor::visit(IfAST& ast) {
    ast.getCond()->accept(*this);
    auto newcond = stack_pop_ptr();
    ast.getThen()->accept(*this);
    auto newthen = stack_pop_ptr();
    ast.getElse()->accept(*this);
    auto newelse = stack_pop_ptr();
    auto newast = std::make_unique<IfAST>(std::move(newcond),std::move(newthen),std::move(newelse));
    res_stack.push(std::move(newast));
};

void TypeInferVisitor::visit(ReturnAST& ast) {

    ast.getExpr()->accept(*this);
    res_stack.push(std::make_unique<ReturnAST>(stack_pop_ptr()));
}
void TypeInferVisitor::visit(ForAST& ast) {
    env = env->createNewChild("forloop"+std::to_string(envcount));
    envcount++;
    ast.getVar()->accept(*this);
    auto newvar = stack_pop_ptr();
    ast.getIterator()->accept(*this);
    auto newiter = stack_pop_ptr();
    ast.getExpression()->accept(*this);
    auto newexpr = stack_pop_ptr();
    auto newast = std::make_unique<IfAST>(std::move(newvar),std::move(newiter),std::move(newexpr));
    res_stack.push(std::move(newast));
    env = env->getParent();
}
void TypeInferVisitor::visit(DeclarationAST& ast) {
    //will not be called
}
void TypeInferVisitor::visit(TimeAST& ast) {
    ast.getExpr()->accept(*this);
    auto newexpr = stack_pop_ptr();
    ast.getTime()->accept(*this);
    auto newtime =stack_pop_ptr();
    auto newast = std::make_unique<TimeAST>(std::move(newexpr),std::move(newtime));
    res_stack.push(std::move(newast));
}

void TypeInferVisitor::visit(StructAST& ast){
  auto newast = std::make_unique<StructAST>();//make empty
  for(auto& [key,val]: ast.map){
      val->accept(*this);
      key->accept(*this);
      newast->addPair(stack_pop_ptr(),stack_pop_ptr());
  }
  res_stack.push(std::move(newast));
}
void TypeInferVisitor::visit(StructAccessAST& ast){
  ast.getVal()->accept(*this);
  ast.getKey()->accept(*this);
  auto newast = std::make_unique<StructAccessAST>(stack_pop_ptr(),stack_pop_ptr());
  res_stack.push(std::move(newast));
}


}//namespace mimium
