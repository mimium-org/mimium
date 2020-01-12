#include "knormalize_visitor.hpp"

#include "ast.hpp"
#include "builtin_fn_types.hpp"

namespace mimium {

KNormalizeVisitor::KNormalizeVisitor(
    std::shared_ptr<TypeInferVisitor> typeinfer_init)
    : var_counter(0) {
  typeinfer = typeinfer_init;
  init();
}

// KNormalizeVisitor::~KNormalizeVisitor(){

// }
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
  while (!res_stack.empty()) {
    res_stack.pop();  // make stack empty
  }
}
// AST_Ptr KNormalizeVisitor::insertAssign(AST_Ptr ast) {
//   if (ast->getid() == OP) {
//     auto tmpsymbol =
//         std::make_shared<LvarAST>(makeNewName());
//     ast->accept(*this);

//     auto assign = std::make_unique<AssignAST>(tmpsymbol, ast);  //
//     recursive!! var_counter++; current_context->appendAST(std::move(assign));
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
  auto nextlhs = stackPopStr();
  ast.rhs->accept(*this);
  auto nextrhs = stackPopStr();
  Instructions newinst = std::make_shared<OpInst>(
      name, ast.getOpStr(), std::move(nextlhs), std::move(nextrhs));
  currentblock->addInst(newinst);
  res_stack_str.push(name);
}

void KNormalizeVisitor::visit(AssignAST& ast) {
  auto name = ast.getName()->getVal();
  auto body = ast.getBody();
  auto it = std::find(lvar_list.cbegin(), lvar_list.cend(), name);

  bool is_overwrite = it != lvar_list.cend();
  if (is_overwrite) {
    body->accept(*this);  // result is stored into res_stack
    auto newname = stackPopStr();
    auto type = typeinfer->getEnv().find(name);
    typeinfer->getEnv().emplace(newname,type );
    tmpname = name;
    auto assign = std::make_shared<AssignInst>(getVarName(), newname);
    assign->type = type;
    Instructions newinst = assign;
    currentblock->addInst(newinst);

  } else {
    tmpname = name;
    body->accept(*this);
    if (body->getid() == RVAR) {  // case of simply copying value
      auto rvar = std::static_pointer_cast<RvarAST>(body);
      rvar->accept(*this);
      auto newname = getVarName();
      auto targetname = stackPopStr();
      typeinfer->getEnv().emplace(newname,
                                  typeinfer->getEnv().find(targetname));

      Instructions newinst = std::make_shared<RefInst>(newname, targetname);
      currentblock->addInst(newinst);
    }
    lvar_list.push_back(name);
  }
}

void KNormalizeVisitor::visit(NumberAST& ast) {
  auto name = getVarName();
  Instructions newinst = std::make_shared<NumberInst>(name, ast.getVal());
  currentblock->addInst(newinst);
  res_stack_str.push(name);
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
  typeinfer->tmpfname = name;
  ast.accept(*typeinfer);
  auto newinst = std::make_shared<FunInst>(name, std::move(newargs),
                                           typeinfer->getLastType());
  Instructions res = newinst;
  currentblock->indent_level++;
  currentblock->addInst(res);
  newinst->body->indent_level = currentblock->indent_level;
  currentblock = newinst->body;  // move context
  ast.getBody()->accept(*this);
  currentblock = tmpcontext;  // switch back context
  --currentblock->indent_level;
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
            typeinfer->getEnv().find(
                std::static_pointer_cast<RvarAST>(elems[0])->getVal()));
        break;
      default:
        res = false;
    }
  }
  return res;
}
void KNormalizeVisitor::visit(FcallAST& ast) {
  auto fname = ast.getFname()->getVal();
  auto newname = getVarName();
  auto args = ast.getArgs();
  std::deque<std::string> newarg;
  for (auto& arg : args->getElements()) {
    arg->accept(*this);
    arg->accept(*typeinfer);
    newarg.push_back(stackPopStr());
  }
  ast.accept(*typeinfer);
  auto tm = builtin::types_map;
  auto fnkind = (tm.find(fname) != tm.end()) ? EXTERNAL : CLOSURE;
  Instructions newinst =
      std::make_shared<FcallInst>(newname, fname, std::move(newarg), fnkind,
                                  typeinfer->getLastType(), isArgTime(*args));
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

  auto newinst = std::make_shared<ArrayInst>(newname, std::move(newelem));
  Instructions res = newinst;
  currentblock->addInst(res);
  res_stack_str.push(newname);
}
void KNormalizeVisitor::visit(ArrayAccessAST& ast) {  // access index may be
                                                      // expr
  ast.getIndex()->accept(*this);
  auto newname = getVarName();
  auto newinst = std::make_shared<ArrayAccessInst>(
      newname, ast.getName()->getVal(), stackPopStr());
  Instructions res = newinst;

  currentblock->addInst(res);
  res_stack_str.push(newname);
}
void KNormalizeVisitor::visit(IfAST& ast) {
  auto tmpcontext = currentblock;
  ast.getCond()->accept(*this);
  auto newname = getVarName();
  auto newinst = std::make_shared<IfInst>(newname, stackPopStr());
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
void KNormalizeVisitor::visit(ReturnAST& ast) {
  ast.getExpr()->accept(*this);
  ast.accept(*typeinfer);
  auto newname = getVarName();
  Instructions newinst = std::make_shared<ReturnInst>(newname, stackPopStr(),
                                                      typeinfer->getLastType());
  currentblock->addInst(newinst);
  res_stack_str.push(newname);
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
  ast.accept(*typeinfer);
  auto newinst = std::make_shared<TimeInst>(
      newname, stackPopStr(), stackPopStr(), typeinfer->getLastType());
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