/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
 
#include "compiler/knormalize_visitor.hpp"

namespace mimium {

KNormalizeVisitor::KNormalizeVisitor(TypeInferVisitor& typeinfer)
    : typeinfer(typeinfer), var_counter(0) {
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
  Instructions newinst =
      OpInst(name, ast.getOpStr(), std::move(nextlhs), std::move(nextrhs));
  currentblock->addInst(newinst);
  res_stack_str.push(name);
  typeinfer.getEnv().emplace(name, types::Float());
}
void KNormalizeVisitor::insertOverWrite(AST_Ptr body, const std::string& name) {
  body->accept(*this);
  auto newname = stackPopStr();
  auto type = type_stack.top();
  typeinfer.getEnv().emplace(newname, type);
  tmpname = name;
  auto assign = AssignInst(getVarName(), newname, type);
  Instructions newinst = assign;
  currentblock->addInst(newinst);
}
void KNormalizeVisitor::insertAlloca(AST_Ptr body, const std::string& name) {
  auto& type = type_stack.top();
  Instructions alloca = AllocaInst(name, type);
  currentblock->addInst(alloca);
  typeinfer.getEnv().emplace(name, type);
}
void KNormalizeVisitor::insertRef(AST_Ptr body, const std::string& name) {
  auto& type = type_stack.top();
  auto reftype = types::Ref(type);
  auto rvar = std::static_pointer_cast<RvarAST>(body);
  auto targetname = rvar->getVal();
  typeinfer.getEnv().emplace(name, reftype);
  Instructions newinst = RefInst(name, targetname);
  currentblock->addInst(newinst);
}
void KNormalizeVisitor::visit(AssignAST& ast) {
  auto name = ast.getName()->getVal();
  auto body = ast.getBody();
  auto& type = typeinfer.getEnv().find(name);
  type_stack.push(type);
  auto it = std::find(lvar_list.cbegin(), lvar_list.cend(), name);

  bool is_overwrite = it != lvar_list.cend();
  if (is_overwrite) {
    insertOverWrite(body, name);
  } else {
    switch (body->getid()) {
      case RVAR:
        insertRef(body, name);
        break;
      case SELF:
        insertAlloca(body, name);
        tmpname = name;
        insertOverWrite(body, name);
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
  type_stack.pop();
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
void KNormalizeVisitor::visit(SelfAST& ast) {
  //   auto assign = AssignInst(getVarName(), "self", type_stack.top());
  // Instructions newinst = assign;
  // currentblock->addInst(newinst);
  res_stack_str.push("self");
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
  FunInst newinst(name, std::move(newargs), type_stack.top(), ast.isrecursive);
  typeinfer.getEnv().emplace(name, type_stack.top());

  newinst.hasself = ast.hasself;
  Instructions res = newinst;
  ++currentblock->indent_level;
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
        res = rv::holds_alternative<types::Time>(typeinfer.getEnv().find(
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
    // arg->accept(typeinfer);
    newarg.push_back(stackPopStr());
  }
  ast.accept(typeinfer);
  auto lasttype = typeinfer.getLastType();
  auto fnkind =
      (LLVMBuiltin::ftable.find(resfname) != LLVMBuiltin::ftable.end())
          ? EXTERNAL
          : CLOSURE;
  // auto type = std::holds_alternative<types::Void>(lasttype) ? lasttype: type_stack.top();
  Instructions newinst = FcallInst(newname, resfname, std::move(newarg), fnkind,
                                   lasttype, isArgTime(*args));

  currentblock->addInst(newinst);
  typeinfer.getEnv().emplace(newname, lasttype);

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

  ArrayInst newinst(newname, std::move(newelem));
  Instructions res = newinst;
  currentblock->addInst(res);
  res_stack_str.push(newname);
}
void KNormalizeVisitor::visit(ArrayAccessAST& ast) {  // access index may be
                                                      // expr
  ast.getIndex()->accept(*this);
  auto newname = getVarName();
  ArrayAccessInst newinst(newname, ast.getName()->getVal(), stackPopStr());
  Instructions res = newinst;

  currentblock->addInst(res);
  res_stack_str.push(newname);
}
void KNormalizeVisitor::visit(IfAST& ast) {
  auto tmpcontext = currentblock;
    auto newname = getVarName();
  ast.getCond()->accept(*this);
  auto condname =  stackPopStr();
    ast.getThen()->accept(*this);
    auto thenval =stackPopStr();
    ast.getElse()->accept(*this);
    auto elseval = stackPopStr();
  if(ast.isexpr){
    Instructions res = FcallInst(newname,"ifexpr",{condname,thenval,elseval},EXTERNAL);
    currentblock->addInst(res);
    res_stack_str.push(newname);
  }else{
  IfInst newinst(newname, condname);
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
}
void KNormalizeVisitor::visit(ReturnAST& ast) {
  ast.getExpr()->accept(*this);
  auto newname = stackPopStr();
  ast.accept(typeinfer);
  auto type = typeinfer.getLastType();
  typeinfer.getEnv().emplace(newname, type);
  Instructions newinst = ReturnInst("ret", newname, type);
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
  auto t = typeinfer.getLastType();
  TimeInst newinst(newname, stackPopStr(), stackPopStr(), t);
  Instructions res = newinst;

  currentblock->addInst(res);
  res_stack_str.push(newname);
  typeinfer.getEnv().emplace(newname, t);
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