#include "compiler/knormalize_visitor.hpp"


namespace mimium {

KNormalizeVisitor::KNormalizeVisitor(TypeInferVisitor& typeinfer)
    :typeinfer(typeinfer), var_counter(0) {
  init();
}
std::string KNormalizeVisitor::makeNewName() {
  return "k" + std::to_string(++var_counter);
}
std::string KNormalizeVisitor::getVarName() {
  auto copiedname = tmpname;
  if (copiedname.empty()) {
    copiedname = makeNewName();
  }
  tmpname = "";  // reset
  return copiedname;
}

void KNormalizeVisitor::init() {
  var_counter = 0;
  rootblock.reset();
  rootblock = std::make_shared<MIRblock>("main");
  currentblock = rootblock;
  current_context = nullptr;

}

void KNormalizeVisitor::visit(ListAST& ast) {
  auto tempctx = current_context;
  current_context = std::make_shared<ListAST>();
  for (auto& elem : ast.getElements()) {
    elem->accept(*this);
  }

}

void KNormalizeVisitor::visit(OpAST& ast) {
  auto name = getVarName();
  ast.lhs->accept(*this);  // recursively visit
  auto nextlhs = stackPopStr();
  ast.rhs->accept(*this);
  auto nextrhs = stackPopStr();
  Instructions newinst =OpInst(
      name, ast.getOpStr(), std::move(nextlhs), std::move(nextrhs));
  currentblock->addInst(newinst);
  res_stack_str.push(name);
  typeinfer.getEnv().emplace(name, types::Float());

}
void KNormalizeVisitor::insertOverWrite(AST_Ptr body, const std::string& name) {
  body->accept(*this);  
  auto newname = stackPopStr();
  auto type = typeinfer.getEnv().find(name);
  typeinfer.getEnv().emplace(newname, type);
  tmpname = name;
  auto assign = AssignInst(getVarName(), newname);
  assign.type = type;
  Instructions newinst = assign;
  currentblock->addInst(newinst);
}
void KNormalizeVisitor::insertAlloca(AST_Ptr body, const std::string& name) {
  Instructions alloca =AllocaInst(name, typeinfer.getEnv().find(name));
  currentblock->addInst(alloca);
}
void KNormalizeVisitor::insertRef(AST_Ptr body, const std::string& name) {
  auto rvar = std::static_pointer_cast<RvarAST>(body);
  auto targetname = rvar->getVal();
  typeinfer.getEnv().emplace(name, typeinfer.getEnv().find(targetname));
  Instructions newinst = RefInst(name, targetname);
  currentblock->addInst(newinst);
}
void KNormalizeVisitor::visit(AssignAST& ast) {
  auto name = ast.getName()->getVal();
  auto body = ast.getBody();
  auto it = std::find(lvar_list.cbegin(), lvar_list.cend(), name);

  bool is_overwrite = it != lvar_list.cend();
  if (is_overwrite) {
    insertOverWrite(body, name);
  } else {
    switch (body->getid()) {
      case RVAR:
        insertRef(body, name);
        break;
      case LAMBDA:  // do not insert allocate,,but how closure is
        tmpname = name;
        body->accept(*this);
        break;
      default:
        insertAlloca(body, name);
        tmpname = name;
        body->accept(*this);
        break;
    }
    lvar_list.push_back(name);
  }
}

void KNormalizeVisitor::visit(NumberAST& ast) {
  auto name = getVarName();
  Instructions newinst = NumberInst(name, ast.getVal());
  currentblock->addInst(newinst);
  res_stack_str.push(name);
  typeinfer.getEnv().emplace(name, types::Float());
}
void KNormalizeVisitor::visit(LvarAST& ast) {
  res_stack_str.push(ast.getVal());
}
void KNormalizeVisitor::visit(RvarAST& ast) {
  res_stack_str.push(ast.getVal());
}
void KNormalizeVisitor::visit(LambdaAST& ast) {
  auto tmpcontext = currentblock;
  auto name = getVarName();
  auto args = ast.getArgs()->getElements();
  std::deque<std::string> newargs;
  for (auto& arg : args) {
    arg->accept(*this);
    newargs.push_back(stackPopStr());
  }
  // typeinfer.tmpfname = name;
  ast.accept(typeinfer);
  FunInst newinst (name, std::move(newargs), typeinfer.getLastType(), ast.isrecursive);
  Instructions res = newinst;
  currentblock->indent_level++;
  currentblock->addInst(res);
  newinst.body->indent_level = currentblock->indent_level;
  currentblock = newinst.body;  // move context
  ast.getBody()->accept(*this);
  currentblock = tmpcontext;  // switch back context
  --currentblock->indent_level;
  res_stack_str.push(name);
}
bool KNormalizeVisitor::isArgTime(FcallArgsAST& args) {
  auto& elems = args.getElements();
  bool res = false;

  if (!elems.empty()) {
    auto id = elems[0]->getid();
    switch (id) {
      case TIME:
        res = true;
        break;
      case RVAR:
        res = std::holds_alternative<recursive_wrapper<types::Time>>(
            typeinfer.getEnv().find(
                std::static_pointer_cast<RvarAST>(elems[0])->getVal()));
        break;
      default:
        res = false;
    }
  }
  return res;
}
void KNormalizeVisitor::visit(FcallAST& ast) {
  ast.getFname()->accept(*this);
  auto resfname = stackPopStr();  
  auto newname = getVarName();
  auto args = ast.getArgs();
  std::deque<std::string> newarg;
  for (auto& arg : args->getElements()) {
    arg->accept(*this);
    arg->accept(typeinfer);
    newarg.push_back(stackPopStr());
  }
  ast.accept(typeinfer);
  auto fnkind = (LLVMBuiltin::ftable.find(resfname) != LLVMBuiltin::ftable.end())
                    ? EXTERNAL
                    : CLOSURE;
  Instructions newinst = FcallInst(newname, resfname, std::move(newarg), fnkind,
                                  typeinfer.getLastType(), isArgTime(*args));
  currentblock->addInst(newinst);
  res_stack_str.push(newname);
};
void KNormalizeVisitor::visit(FcallArgsAST& ast) {}
void KNormalizeVisitor::visit(ArgumentsAST& ast) {}
void KNormalizeVisitor::visit(ArrayAST& ast) {
  std::deque<std::string> newelem;
  for (auto& elem : ast.getElements()) {
    elem->accept(*this);
    newelem.push_back(stackPopStr());
  }
  auto newname = getVarName();

  ArrayInst newinst ( newname, std::move(newelem));
  Instructions res = newinst;
  currentblock->addInst(res);
  res_stack_str.push(newname);
}
void KNormalizeVisitor::visit(ArrayAccessAST& ast) {  // access index may be
                                                      // expr
  ast.getIndex()->accept(*this);
  auto newname = getVarName();
  ArrayAccessInst newinst (      newname, ast.getName()->getVal(), stackPopStr());
  Instructions res = newinst;

  currentblock->addInst(res);
  res_stack_str.push(newname);
}
void KNormalizeVisitor::visit(IfAST& ast) {
  auto tmpcontext = currentblock;
  ast.getCond()->accept(*this);
  auto newname = getVarName();
  IfInst newinst (newname, stackPopStr());
  Instructions res = newinst;
  currentblock->indent_level++;
  newinst.thenblock->indent_level = currentblock->indent_level;
  newinst.elseblock->indent_level = currentblock->indent_level;
  currentblock->addInst(res);
  currentblock = newinst.thenblock;
  ast.getThen()->accept(*this);
  currentblock = newinst.elseblock;
  ast.getElse()->accept(*this);
  res_stack_str.push(newname);
  currentblock = tmpcontext;
  currentblock->indent_level--;
}
void KNormalizeVisitor::visit(ReturnAST& ast) {
  ast.getExpr()->accept(*this);
  auto newname = stackPopStr();
  auto type =typeinfer.getLastType();
    typeinfer.getEnv().emplace(newname, type);
  ast.accept(typeinfer);
  Instructions newinst = ReturnInst("ret", newname,type);
  currentblock->addInst(newinst);
}
void KNormalizeVisitor::visit(ForAST& ast) {
  //後回し
}
void KNormalizeVisitor::visit(DeclarationAST& ast) {
  current_context->appendAST(std::make_unique<DeclarationAST>(ast));
}
void KNormalizeVisitor::visit(TimeAST& ast) {
  //ここ考えどこだな　functionにパスするときのなんかもここで処理するかMIRの先で処理するか
  //とりあえずこの先で処理します
  auto newname = getVarName();

  ast.getTime()->accept(*this);
  ast.getExpr()->accept(*this);
  ast.accept(typeinfer);
  TimeInst newinst (newname, stackPopStr(), stackPopStr(), typeinfer.getLastType());
  Instructions res = newinst;

  currentblock->addInst(res);
  res_stack_str.push(newname);
}

void KNormalizeVisitor::visit(StructAST& ast) {
  // will not be used for now,,,
}
void KNormalizeVisitor::visit(StructAccessAST& ast) {
  // will not be used for now,,,

  // ast.getVal()->accept(*this);
  // ast.getKey()->accept(*this);
  // auto newast =
  // std::make_unique<StructAccessAST>(stack_pop_ptr(),stack_pop_ptr());
  // res_stack.push(std::move(newast));
}

std::shared_ptr<MIRblock> KNormalizeVisitor::getResult() { return rootblock; }

}  // namespace mimium