/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/knormalize_visitor.hpp"

namespace mimium {

KNormalizeVisitor::KNormalizeVisitor(TypeInferVisitor& typeinfer)
    : typeinfer(typeinfer), var_counter(0) {
  init();
}
KNormalizeVisitor::KNormalizeVisitor() : var_counter(0) { init(); }

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
  // Instructions newinst =
  //     OpInst(name, ast.getOpStr(), std::move(nextlhs), std::move(nextrhs));
  // currentblock->addInst(newinst);
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
void KNormalizeVisitor::visit(StringAST& ast) {
  auto name = getVarName();
  Instructions newinst = StringInst(name, ast.val);
  currentblock->addInst(newinst);
  res_stack_str.push(name);
  typeinfer.getEnv().emplace(name, types::String());
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
// bool KNormalizeVisitor::isArgTime(FcallArgsAST& args) {
//   auto& elems = args.getElements();
//   bool res = false;

//   if (!elems.empty()) {
//     auto id = elems[0]->getid();
//     switch (id) {
//       case TIME:
//         res = true;
//         break;
//       case RVAR:
//         res = rv::holds_alternative<types::Time>(typeinfer.getEnv().find(
//             std::static_pointer_cast<RvarAST>(elems[0])->getVal()));
//         break;
//       default:
//         res = false;
//     }
//   }
//   return res;
// }
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
  std::optional<std::string> time = std::nullopt;
  if (ast.time != nullptr) {
    ast.time->accept(*this);
    time = stackPopStr();
  }
  auto fnkind =
      (LLVMBuiltin::ftable.find(resfname) != LLVMBuiltin::ftable.end())
          ? EXTERNAL
          : CLOSURE;
  if (newname == "mimium_getnow") {
    fnkind = DIRECT;
  }
  // auto type = std::holds_alternative<types::Void>(lasttype) ? lasttype:
  // type_stack.top();
  Instructions newinst = FcallInst(newname, resfname, std::move(newarg), fnkind,
                                   lasttype, std::move(time));

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
  auto newname = getVarName();
  ast.getIndex()->accept(*this);

  ArrayAccessInst newinst(newname, ast.getName()->getVal(), stackPopStr());
  Instructions res = newinst;

  currentblock->addInst(res);
  res_stack_str.push(newname);
}
void KNormalizeVisitor::visit(IfAST& ast) {
  auto tmpcontext = currentblock;
  auto newname = getVarName();
  ast.getCond()->accept(*this);
  auto condname = stackPopStr();
  ast.getThen()->accept(*this);
  auto thenval = stackPopStr();
  ast.getElse()->accept(*this);
  auto elseval = stackPopStr();
  if (ast.isexpr) {
    Instructions res =
        FcallInst(newname, "ifexpr", {condname, thenval, elseval}, EXTERNAL);
    currentblock->addInst(res);
    res_stack_str.push(newname);
  } else {
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
// void KNormalizeVisitor::visit(TimeAST& ast) {
//   //ここ考えどこだな　functionにパスするときのなんかもここで処理するかMIRの先で処理するか
//   //とりあえずこの先で処理します
//   auto newname = getVarName();

//   ast.getTime()->accept(*this);
//   ast.getExpr()->accept(*this);
//   ast.accept(typeinfer);
//   auto t = typeinfer.getLastType();
//   TimeInst newinst(newname, stackPopStr(), stackPopStr(), t);
//   Instructions res = newinst;

//   currentblock->addInst(res);
//   res_stack_str.push(newname);
//   typeinfer.getEnv().emplace(newname, t);
// }

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

// new knormalizer(mir generator)

std::shared_ptr<MIRblock> MirGenerator::generate(newast::Statements& topast) {
  auto lvarid = generateBlock(topast, "root");
  return ctx;
}

std::string MirGenerator::makeNewName() {
  auto res = lvar_holder.value_or("k" + std::to_string(varcounter++));
  lvar_holder = std::nullopt;
  return res;
}

std::pair<lvarid, std::shared_ptr<MIRblock>> MirGenerator::generateBlock(
    newast::Statements stmts, std::string label) {
  auto tmpctx = ctx;
  auto functx = std::make_shared<MIRblock>(label);
  ctx = functx;
  lvarid laststmt;
  for (auto&& s : stmts) {
    laststmt = std::visit(statementvisitor, *s);
  }
  ctx = tmpctx;
  return {laststmt, functx};
}

using ExprKnormVisitor = MirGenerator::ExprKnormVisitor;
using StatementKnormVisitor = MirGenerator::StatementKnormVisitor;

lvarid ExprKnormVisitor::operator()(newast::Op& ast) {
  auto lhsname =
      ast.lhs.has_value() ? std::visit(*this, *ast.lhs.value()).first : "";
  return mirgen.emplace(OpInst(mirgen.makeNewName(), ast.op, lhsname,
                               std::visit(*this, *ast.rhs).first),
                        types::Float());
}
lvarid ExprKnormVisitor::operator()(newast::Number& ast) {
  return mirgen.emplace(NumberInst(mirgen.makeNewName(), ast.value),
                        types::Float());
}
lvarid ExprKnormVisitor::operator()(newast::String& ast) {
  return mirgen.emplace(StringInst(mirgen.makeNewName(), ast.value),
                        types::String());
}
lvarid ExprKnormVisitor::operator()(newast::Rvar& ast) {
  if (ast.value == "now") {
    return mirgen.emplace(
        FcallInst(mirgen.makeNewName(), "mimium_getnow", {}, DIRECT),
        types::Float());
  }
  return std::pair(ast.value, mirgen.typeenv.find(ast.value));
}
lvarid ExprKnormVisitor::operator()(newast::Self& /*ast*/) {
  lvarid res = {"self", mirgen.selftype_stack.top()};
  mirgen.selftype_stack.pop();
  return res;
}
lvarid ExprKnormVisitor::operator()(newast::Lambda& ast) {
  auto label = mirgen.makeNewName();

  auto fun = FunInst(
      label,
      mirgen.transformArgs(ast.args.args, std::deque<std::string>{},
                           [&](newast::Lvar& lvar) { return lvar.value; }));
  auto* typeptr = mirgen.typeenv.tryFind(label);
  auto [lvar, ctx] = mirgen.generateBlock(ast.body, label);
  fun.body = ctx;
  types::Value type =
      (typeptr != nullptr)
          ? *typeptr
          : types::Function(
                lvar.second,
                mirgen.transformArgs(ast.args.args, std::vector<types::Value>{},
                                     [&](newast::Lvar& lvar) {
                                       return mirgen.typeenv.find(lvar.value);
                                     }));
  return mirgen.emplace(std::move(fun), std::move(type));
}
lvarid ExprKnormVisitor::operator()(newast::Fcall& ast,
                                    std::optional<std::string> when) {
  auto [fname, type] = std::visit(*this, *ast.fn);
  auto* rettype_ptr =
      std::get_if<types::rFunction>(&mirgen.typeenv.find(fname));
  types::Value rettype =
      (rettype_ptr == nullptr) ? types::Value(types::None()) : *rettype_ptr;
  auto fnkind = MirGenerator::isExternalFun(fname) ? EXTERNAL : CLOSURE;
  return mirgen.emplace(FcallInst(
      mirgen.makeNewName(), fname,
      mirgen.transformArgs(ast.args.args, std::deque<std::string>{},
                           [&](auto expr) { return std::visit(*this, *expr).first; }),
      fnkind, rettype, when));
}
lvarid ExprKnormVisitor::operator()(newast::Time& ast) {
  return (*this)(ast.fcall, std::visit(*this, *ast.when).first);
}
lvarid ExprKnormVisitor::operator()(newast::Struct& ast) {
  // TODO
  return lvarid{};
}
lvarid ExprKnormVisitor::operator()(newast::StructAccess& ast) {
  // TODO
  return lvarid{};
}
lvarid ExprKnormVisitor::operator()(newast::ArrayInit& ast) {
  // TODO
  return lvarid{};
}
lvarid ExprKnormVisitor::operator()(newast::ArrayAccess& ast) {
  auto [arrname, arrtype] = std::visit(*this, *ast.array);
  auto* arrtype_ptr = std::get_if<types::rArray>(&arrtype);
  types::Value rettype;
  if (arrtype_ptr != nullptr) {
    rettype = arrtype_ptr->getraw().elem_type;
  } else {
    throw std::runtime_error(
        "[] operator cannot be used for other than array type");
  }
  return mirgen.emplace(ArrayAccessInst(mirgen.makeNewName(), arrname,
                                        std::visit(*this, *ast.index).first),
                        std::move(rettype));
}
lvarid ExprKnormVisitor::operator()(newast::Tuple& ast) {
  // TODO
  return lvarid{};
}

lvarid StatementKnormVisitor::operator()(newast::Assign& ast) {
  mirgen.lvar_holder = ast.lvar.value;
  bool is_overwrite = mirgen.isOverWrite(ast.lvar.value);
  auto [rinstname, type] = std::visit(mirgen.exprvisitor, *ast.expr);
  Instructions res = NumberInst("", 0);
  if (!is_overwrite) {
    auto [allocaname, allcatype] = mirgen.emplace(
        AllocaInst(ast.lvar.value + "_ptr"), types::Pointer(type));
  }
  mirgen.lvarlist.emplace_back(ast.lvar.value);
  return mirgen.emplace(AssignInst(ast.lvar.value, rinstname),
                        types::Value(type));
}
lvarid StatementKnormVisitor::operator()(newast::Return& ast) {
  auto [ret, type] = std::visit(mirgen.exprvisitor, *ast.value);
  return mirgen.emplace(
      ReturnInst(mirgen.makeNewName(), ret, type),types::Value(type));
}
// Instructions StatementKnormVisitor::operator()(newast::Declaration& ast){}
lvarid StatementKnormVisitor::operator()(newast::For& ast) {
  // TODO
  return lvarid{};
}
lvarid StatementKnormVisitor::operator()(newast::If& ast) {
  auto lvname = mirgen.makeNewName();
  auto ifinst = IfInst(lvname, std::visit(mirgen.exprvisitor, *ast.cond).first);
  auto [thenvarid, thenblock] =
      mirgen.generateBlock(ast.then_stmts, lvname + "$then");
  ifinst.thenblock = thenblock;
  ifinst.elseblock =
      ast.else_stmts.has_value()
          ? mirgen.generateBlock(ast.else_stmts.value(), lvname + "$else")
                .second
          : nullptr;
  return mirgen.emplace(std::move(ifinst), types::Value(thenvarid.second));
}
lvarid StatementKnormVisitor::operator()(newast::ExprPtr& ast) {
  return std::visit(mirgen.exprvisitor, *ast);
}

}  // namespace mimium