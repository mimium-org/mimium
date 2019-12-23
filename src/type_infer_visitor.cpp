#include "type_infer_visitor.hpp"
#include <variant>
#include "type.hpp"

namespace mimium {
TypeInferVisitor::TypeInferVisitor() : has_return(false) { 
  for(const auto& [key,val]: builtin::types_map){
  typeenv.env.emplace(key,val);
  }
 }

void TypeInferVisitor::init() { has_return = false; }
void TypeInferVisitor::visit(LvarAST& ast) { res_stack = ast.type; }

void TypeInferVisitor::visit(RvarAST& ast) {
  auto it = typeenv.env.find(ast.getVal());
  res_stack = it->second;
}
void TypeInferVisitor::visit(OpAST& ast) {
  ast.lhs->accept(*this);
  types::Value lhstype = res_stack;
  ast.rhs->accept(*this);
  types::Value rhstype = res_stack;
  if (lhstype.index() != rhstype.index()) {
    std::logic_error("type of lhs and rhs is not matched");
  }
  types::Float f;
  res_stack = f;
}
void TypeInferVisitor::visit(ListAST& ast) {
  for (auto& line : ast.getElements()) {
    line->accept(*this);
  }
}
void TypeInferVisitor::visit(NumberAST& /*ast*/) { res_stack = types::Float(); }
void TypeInferVisitor::visit(AssignAST& ast) {
  ast.getBody()->accept(*this);
  typeenv.env[ast.getName()->getVal()] = res_stack;
}
void TypeInferVisitor::visit(ArgumentsAST& ast) {
  std::vector<types::Value> argtypes;
  for (const auto& a : ast.getElements()) {
    a->accept(*this);
    auto aval = std::static_pointer_cast<LvarAST>(a);
    typeenv.env[aval->getVal()] = res_stack;
    argtypes.push_back(res_stack);
  }
  types::Void v;
  types::Function f(std::move(argtypes), v);
  res_stack = std::move(f);
}
void TypeInferVisitor::visit(FcallArgsAST& ast) {
  //
}
void TypeInferVisitor::visit(ArrayAST& ast) {
  auto tmpres = res_stack;
  for (const auto& v : ast.getElements()) {
    v->accept(*this);
    if (tmpres.index() != res_stack.index()) {
      throw std::logic_error("array contains different types.");
    }
    tmpres = res_stack;
  }
  res_stack = types::Array(tmpres);
}
void TypeInferVisitor::visit(ArrayAccessAST& ast) {
  auto it = typeenv.env.find(ast.getName()->getVal());
  types::Value res;
  if (it != typeenv.env.end()) {
    types::Array arr =
        std::get<recursive_wrapper<types::Array>>(it->second);  // implicit cast
    arr.getElemType();
  } else {
    throw std::logic_error("could not find accessed array");
  }
  res_stack = res;
}
bool TypeInferVisitor::checkArg(const types::Value& fnarg,const types::Value& givenarg){
  bool res;
    if(std::holds_alternative<recursive_wrapper<types::Time>>(givenarg)){
      types::Time v =std::get<recursive_wrapper<types::Time>>(givenarg);
      res = fnarg.index() == v.val.index(); //currently
    }else{
      res = fnarg.index() == givenarg.index();
    }
    return res;
  }


void TypeInferVisitor::visit(FcallAST& ast) {
  auto fname = ast.getFname()->getVal();
  auto it = typeenv.env.find(fname);
  if(it == typeenv.env.end()){
    // Logger::debug_log("function "+ fname+ " could not be found",Logger::ERROR);
    throw std::logic_error("function "+ fname+ " could not be found");
  }
  auto [name, type] = *it;
  types::Function fn = std::get<recursive_wrapper<types::Function>>(type);

  types::Value res;
  auto fnargtypes = fn.getArgTypes();
  auto args = ast.getArgs()->getElements();

  std::vector<types::Value> argtypes;
  bool checkflag = false;
  for (int i = 0; i < args.size(); i++) {
    args[i]->accept(*this);
    checkflag |=  checkArg(fnargtypes[i],res_stack);
  }
  if (!checkflag) {
    throw std::invalid_argument("argument types were invalid");
  }
  res = fn.getReturnType();

  res_stack = res;
}
void TypeInferVisitor::visit(LambdaAST& ast) {
  auto args = ast.getArgs();
  args->accept(*this);
  types::Function fntype =
      std::get<recursive_wrapper<types::Function>>(res_stack);
  ast.getBody()->accept(*this);
  types::Value res_type;
  if (has_return) {
    res_type = res_stack;
  } else {
    types::Void t;
    res_type = t;
  }
  fntype.ret_type = res_type;
  res_stack = fntype;
  has_return = false;  // switch back flag
}
void TypeInferVisitor::visit(IfAST& ast) {
  ast.getCond()->accept(*this);

  // auto newcond = stack_pop_ptr();
  ast.getThen()->accept(*this);
  // auto newthen = stack_pop_ptr();
  ast.getElse()->accept(*this);
  // auto newelse = stack_pop_ptr();
  // auto newast =
  // std::make_unique<IfAST>(std::move(newcond),std::move(newthen),std::move(newelse));
  // res_stack.push(std::move(newast));
};

void TypeInferVisitor::visit(ReturnAST& ast) {
  ast.getExpr()->accept(*this);
  has_return = true;
}
void TypeInferVisitor::visit(ForAST& ast) {
  // env = env->createNewChild("forloop"+std::to_string(envcount));
  // envcount++;
  // ast.getVar()->accept(*this);
  // auto newvar = stack_pop_ptr();
  // ast.getIterator()->accept(*this);
  // auto newiter = stack_pop_ptr();
  // ast.getExpression()->accept(*this);
  // auto newexpr = stack_pop_ptr();
  // auto newast =
  // std::make_unique<IfAST>(std::move(newvar),std::move(newiter),std::move(newexpr));
  // res_stack.push(std::move(newast));
  // env = env->getParent();
}
void TypeInferVisitor::visit(DeclarationAST& ast) {
  // will not be called
}
void TypeInferVisitor::visit(TimeAST& ast) {
    types::Time t;
  ast.getExpr()->accept(*this);
  t.val = res_stack;
  ast.getTime()->accept(*this);
  t.time = types::Float();
  res_stack = t;
}

void TypeInferVisitor::visit(StructAST& ast) {}
void TypeInferVisitor::visit(StructAccessAST& ast) {}

}  // namespace mimium
