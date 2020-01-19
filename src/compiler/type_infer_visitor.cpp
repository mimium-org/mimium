#include "compiler/type_infer_visitor.hpp"

#include <variant>

#include "basic/type.hpp"

namespace mimium {
TypeInferVisitor::TypeInferVisitor() : has_return(false) {
  for (const auto& [key, val] : LLVMBuiltin::ftable) {
    typeenv.emplace(key, val.mmmtype);
  }
}

void TypeInferVisitor::init() { has_return = false; }

bool TypeInferVisitor::typeCheck(types::Value& lt, types::Value& rt) {
  bool res = lt.index() == rt.index();
  if (!res) {
    throw std::logic_error("type not matched");
  }
  return res;
}

bool TypeInferVisitor::unify(types::Value& lt, types::Value& rt) {
  bool istvl = types::isTypeVar(lt);
  bool istvr = types::isTypeVar(rt);
  bool res = true;
  if (istvl && !istvr) {
    lt = rt;
  } else if (!istvl && istvr) {
    rt = lt;
  } else if (istvl && istvr) {
    std::get<types::TypeVar>(lt).setIndex(
        std::get<types::TypeVar>(rt).getIndex());
  } else {
    res = typeCheck(lt, rt);
  }
  return res;
}

bool TypeInferVisitor::unify(std::string lname, types::Value& rt) {
  return unify(typeenv.find(lname), rt);
}
bool TypeInferVisitor::unify(std::string lname, std::string rname) {
  return unify(typeenv.find(lname), typeenv.find(rname));
}

bool TypeInferVisitor::unifyArg(types::Value& target, types::Value& realarg) {
  if (auto* timeptr = std::get_if<recursive_wrapper<types::Time>>(&realarg)) {
    types::Time time = *timeptr;
    return unify(time.val, target);
  } else {
    return unify(realarg, target);
  }
}

void TypeInferVisitor::visit(AssignAST& ast) {
  auto lvar = ast.getName();
  ast.getName()->accept(*this);
  auto ltype = res_stack;
  typeenv.emplace(lvar->getVal(), ltype);
  auto body = ast.getBody();
  if(body->getid() == LAMBDA){
    tmpfname = lvar->getVal();
  }
  body->accept(*this);
  if (!unify(lvar->getVal(), res_stack)) {
    throw std::logic_error("type " + lvar->getVal() +
                           " did not matched to expression " +
                           ast.getBody()->toString());
  }
}

void TypeInferVisitor::visit(LvarAST& ast) {
  auto& ltype = ast.getType();
  ;
  bool is_specified = !std::holds_alternative<types::None>(ltype);
  bool is_exist = typeenv.exist(ast.getVal());
  if (is_exist && is_specified) {
    if (typeenv.find(ast.getVal()) != ltype) {
      throw std::logic_error("assignment to different type is not allowed");
      res_stack = types::None();
    } else {
      res_stack = ltype;
    }
  } else if (!is_exist && !is_specified) {
    res_stack = typeenv.createNewTypeVar();
  } else if (!is_exist && is_specified) {
    res_stack = ltype;
  } else {  // case of already declared variable that have not specified in
            // source code

    res_stack = typeenv.find(ast.getVal());
    ;
    ;
  }
}

void TypeInferVisitor::visit(RvarAST& ast) {
  res_stack = typeenv.find(ast.getVal());
}
void TypeInferVisitor::visit(OpAST& ast) {
  ast.lhs->accept(*this);
  types::Value lhstype = res_stack;
  ast.rhs->accept(*this);
  types::Value rhstype = res_stack;
  if (unify(lhstype, rhstype)) {
    std::logic_error("type of lhs and rhs is not matched");
  }
  // anyway return type is float for now
  res_stack = types::Float();
}
void TypeInferVisitor::visit(ListAST& ast) {
  for (auto& line : ast.getElements()) {
    line->accept(*this);
  }
}
void TypeInferVisitor::visit(NumberAST& /*ast*/) { res_stack = types::Float(); }

void TypeInferVisitor::visit(ArgumentsAST& ast) {
  //
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
  auto type = typeenv.find(ast.getName()->getVal());
  types::Value res;
  types::Array arr =
      std::get<recursive_wrapper<types::Array>>(type);  // implicit cast
  res = arr.elem_type;

  res_stack = res;
}
bool TypeInferVisitor::checkArg(const types::Value& fnarg,
                                const types::Value& givenarg) {
  bool res;
  if (std::holds_alternative<recursive_wrapper<types::Time>>(givenarg)) {
    const types::Time v = std::get<recursive_wrapper<types::Time>>(givenarg);
    res = fnarg.index() == v.val.index();  // currently
  } else {
    res = fnarg.index() == givenarg.index();
  }
  return res;
}

void TypeInferVisitor::visit(FcallAST& ast) {
  auto fname = ast.getFname()->getVal();
  auto type = typeenv.find(fname);
  types::Function fn = std::get<recursive_wrapper<types::Function>>(type);
  types::Value res;
  auto fnargtypes = fn.getArgTypes();
  auto args = ast.getArgs()->getElements();

  std::vector<types::Value> argtypes;
  bool checkflag = true;
  for (int i = 0; i < args.size(); i++) {
    args[i]->accept(*this);
    checkflag &= unifyArg(fnargtypes[i], res_stack);
  }
  if (!checkflag) {
    throw std::invalid_argument("argument types were invalid");
  }
  res_stack = fn.getReturnType();
  ;
}
void TypeInferVisitor::visit(LambdaAST& ast) {
  auto args = ast.getArgs();
  std::vector<types::Value> argtypes;
  for (const auto& a : args->getElements()) {
    a->accept(*this);
    typeenv.emplace(a->getVal(), res_stack);
    argtypes.push_back(res_stack);
  }
  bool isspecified =
      std::holds_alternative<recursive_wrapper<types::Function>>(ast.type);
  // case of no type specifier
  types::Value res_type;

  if (isspecified) {
    types::Function fntype = std::get<recursive_wrapper<types::Function>>(ast.type);
    fntype.arg_types = argtypes;  // overwrite
    std::string s(tmpfname);
    types::Value f = fntype;
    unify(s,f);
    ast.getBody()->accept(*this);
    auto tmp_res_type = (has_return) ? res_stack : types::Void();
    typeCheck(tmp_res_type, fntype.ret_type);
    res_type = f;
  } else {
    if (ast.isrecursive) {
      throw std::logic_error("recursive function need to specify return type.");
    }
    ast.getBody()->accept(*this);
    types::Value ret_type = (has_return) ? res_stack : types::Void();
    res_type = types::Function(ret_type,argtypes);
  }

  res_stack = res_type;
  tmpfname="";
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
  if(std::holds_alternative<recursive_wrapper<types::Time>>(res_stack)){
    throw std::logic_error("Time type can not be nested.");
  }

  t.val = res_stack;
  ast.getTime()->accept(*this);

  types::Value tmpf = types::Float();
  typeCheck(res_stack,tmpf);
  t.time = types::Float();
  res_stack = t;
}

void TypeInferVisitor::visit(StructAST& ast) {}
void TypeInferVisitor::visit(StructAccessAST& ast) {}

}  // namespace mimium
