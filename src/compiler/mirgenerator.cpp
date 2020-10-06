/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/mirgenerator.hpp"

namespace mimium {

// new knormalizer(mir generator)

mir::blockptr MirGenerator::generate(ast::Statements& topast) {
  auto topblock = ast::Block{ast::DebugInfo{}, topast, std::nullopt};
  auto [lvarid, ctx] = generateBlock(topblock, "root");
  return ctx;
}

std::string MirGenerator::makeNewName() {
  auto res = lvar_holder.value_or("k" + std::to_string(varcounter++));
  lvar_holder = std::nullopt;
  return res;
}

std::pair<lvarid, mir::blockptr> MirGenerator::generateBlock(ast::Block& block, std::string label) {
  int indent = (ctx == nullptr) ? 0 : (ctx->indent_level + 1);
  auto tmpctx = ctx;
  auto functx = mir::makeBlock(label, indent);
  ctx = functx;
  ctx->indent_level = indent;
  lvarid laststmt;
  for (auto&& s : block.stmts) { laststmt = genInst(*s); }
  if (block.expr.has_value()) {
    auto expr = block.expr.value();
    auto newreturn = ast::Return{block.debuginfo, expr};
    laststmt = genInst(newreturn);
  }
  ctx = tmpctx;
  return {laststmt, functx};
}

using ExprKnormVisitor = MirGenerator::ExprKnormVisitor;
using StatementKnormVisitor = MirGenerator::StatementKnormVisitor;
using AssignKnormVisitor = MirGenerator::AssignKnormVisitor;

lvarid ExprKnormVisitor::operator()(ast::Op& ast) {
  auto newname = mirgen.makeNewName();
  auto lhsname = ast.lhs.has_value() ? std::visit(*this, *ast.lhs.value()).first : "";
  auto rhsname = std::visit(*this, *ast.rhs).first;
  return mirgen.emplace(mir::OpInst{{newname}, ast.op, lhsname, rhsname}, types::Float{});
}
lvarid ExprKnormVisitor::operator()(ast::Number& ast) {
  return mirgen.emplace(mir::NumberInst{{mirgen.makeNewName()}, ast.value}, types::Float{});
}
lvarid ExprKnormVisitor::operator()(ast::String& ast) {
  return mirgen.emplace(mir::StringInst{{mirgen.makeNewName()}, ast.value}, types::String{});
}
lvarid ExprKnormVisitor::operator()(ast::Rvar& ast) {
  if (ast.value == "now") {
    return mirgen.emplace(
        mir::FcallInst{
            {mirgen.makeNewName()}, "mimium_getnow", {}, FCALLTYPE::DIRECT, std::nullopt},
        types::Float{});
  }
  return std::pair(ast.value, mirgen.typeenv.find(ast.value));
}
lvarid ExprKnormVisitor::operator()(ast::Self& ast) {
  lvarid res = {"self", ast.type.value_or(types::Float{})};
  return res;
}
lvarid ExprKnormVisitor::operator()(ast::Lambda& ast) {
  auto label = mirgen.makeNewName();
  auto fun = mir::FunInst{{label},
                          mirgen.transformArgs(ast.args.args, std::deque<std::string>{},
                                               [&](ast::DeclVar& lvar) { return lvar.value; }),
                          {}};
  auto* typeptr = mirgen.typeenv.tryFind(label);
  auto [lvar, ctx] = mirgen.generateBlock(ast.body, label);
  fun.body = ctx;
  types::Value type =
      (typeptr != nullptr)
          ? *typeptr
          : types::Function{lvar.second,
                            mirgen.transformArgs(ast.args.args, std::vector<types::Value>{},
                                                 [&](ast::DeclVar& lvar) {
                                                   return mirgen.typeenv.find(lvar.value);
                                                 })};
  return mirgen.emplace(std::move(fun), std::move(type));
}
lvarid MirGenerator::genFcallInst(ast::Fcall& fcall, std::optional<std::string> when) {
  auto [fname, type] = genInst(fcall.fn);
  auto* rettype_ptr = std::get_if<types::rFunction>(&typeenv.find(fname));
  types::Value rettype = (rettype_ptr == nullptr) ? types::None() : rettype_ptr->getraw().ret_type;
  auto fnkind = MirGenerator::isExternalFun(fname) ? EXTERNAL : CLOSURE;
  auto newname = makeNewName();
  auto newargs = transformArgs(fcall.args.args, std::deque<std::string>{},
                               [&](auto expr) { return genInst(expr).first; });
  return emplace(mir::FcallInst{{newname}, fname, newargs, fnkind, when}, rettype);
}
lvarid ExprKnormVisitor::operator()(ast::Fcall& ast, std::optional<std::string> when) {
  return mirgen.genFcallInst(ast, when);
}

lvarid ExprKnormVisitor::operator()(ast::Struct& /*ast*/) {
  // TODO(tomoya)
  return lvarid{};
}
lvarid ExprKnormVisitor::operator()(ast::StructAccess& /*ast*/) {
  // TODO(tomoya)
  return lvarid{};
}
lvarid ExprKnormVisitor::operator()(ast::ArrayInit& ast) {
  std::deque<std::string> newelems;
  types::Value lasttype;
  std::transform(ast.args.begin(), ast.args.end(), std::back_inserter(newelems),
                 [&](ast::ExprPtr e) -> std::string {
                   auto [ename, etype] = std::visit(*this, *e);
                   lasttype = etype;
                   return ename;
                 });
  auto newname = mirgen.makeNewName();
  auto type = types::Array{lasttype};
  auto inst = mir::ArrayInst{{newname}, newelems};
  return mirgen.emplace(inst, type);
}
lvarid ExprKnormVisitor::operator()(ast::ArrayAccess& ast) {
  auto [arrname, arrtype] = std::visit(*this, *ast.array);
  auto* arrtype_ptr = std::get_if<types::rArray>(&arrtype);
  types::Value rettype;
  if (arrtype_ptr != nullptr) {
    rettype = arrtype_ptr->getraw().elem_type;
  } else {
    throw std::runtime_error("[] operator cannot be used for other than array type");
  }
  auto newname = mirgen.makeNewName();
  return mirgen.emplace(
      mir::ArrayAccessInst{{newname}, arrname, std::visit(*this, *ast.index).first},
      std::move(rettype));
}
lvarid ExprKnormVisitor::operator()(ast::Tuple& /*ast*/) {
  // TODO(tomoya)
  return lvarid{};
}

lvarid ExprKnormVisitor::operator()(ast::Block& /*ast*/) {
  static_assert(true, "should be unreachable");
  return lvarid{};
}

std::pair<lvarid, mir::blockptr> MirGenerator::genIfBlock(ast::ExprPtr& block,
                                                          std::string const& label) {
  auto realblock = (rv::holds_alternative<ast::Block>(*block))
                       ? rv::get<ast::Block>(*block)
                       : ast::Block{ast::DebugInfo{}, {}, block};
  return generateBlock(realblock, label);
}

lvarid MirGenerator::genIfInst(ast::If& ast) {
  bool need_alloca = !lvar_holder.has_value();
  auto lvname = makeNewName();
  auto cond = genInst(ast.cond).first;
  auto [thenvarid, thenblock] = genIfBlock(ast.then_stmts, lvname + "$then");
  types::Value rettype = thenvarid.second;
  std::optional<mir::blockptr> elseblock =
      ast.else_stmts.has_value()
          ? std::optional(genIfBlock(ast.else_stmts.value(), lvname + "$else").second)
          : std::nullopt;
  if (need_alloca) { emplace(mir::AllocaInst{{lvname}, rettype}, rettype); }
  return emplace(mir::IfInst{{lvname}, cond, thenblock, elseblock}, thenvarid.second);
}

lvarid ExprKnormVisitor::operator()(ast::If& ast) { return mirgen.genIfInst(ast); }

lvarid StatementKnormVisitor::operator()(ast::If& ast) { return mirgen.genIfInst(ast); }
lvarid StatementKnormVisitor::operator()(ast::Fdef& ast) {
  bool is_overwrite = mirgen.isOverWrite(ast.lvar.value);
  if (!is_overwrite) { mirgen.lvar_holder = ast.lvar.value; }
  return mirgen.exprvisitor(ast.fun);
}

lvarid AssignKnormVisitor::operator()(ast::DeclVar& ast) {
  bool is_overwrite = mirgen.isOverWrite(ast.value);
  if (!is_overwrite) { mirgen.lvar_holder = ast.value; }
  lvarid res = std::visit(mirgen.exprvisitor, *expr);
  auto [rvar, type] = res;
  if (!rv::holds_alternative<types::Function>(type)) {
    if (!is_overwrite) {
      std::string ptrname = ast.value;
      types::Value ptrtype = type;
      auto iter = mirgen.ctx->instructions.insert(std::prev(mirgen.ctx->instructions.end()),
                                                  mir::AllocaInst{{ptrname}, ptrtype});
      // mirgen.typeenv.emplace(ptrname, ptrtype);
      mirgen.lvarlist.emplace_back(ast.value);
    } else {
      res = mirgen.emplace(mir::AssignInst{{ast.value}, rvar, type}, types::Value(type));
    }
  }
  return std::pair(res.first, types::Void{});
}
lvarid AssignKnormVisitor::operator()(ast::ArrayLvar& ast) {
  auto [arrayname, type] = std::visit(mirgen.exprvisitor, *ast.array);
  auto [indexname, indextype] = std::visit(mirgen.exprvisitor, *ast.index);
  auto newlvarname = arrayname + "_lvar";  // todo: avoid namespace conflict
  auto [rvar, rvartype] = std::visit(mirgen.exprvisitor, *expr);
  auto [arrayaccessname, arracctype] =
      mirgen.emplace(mir::ArrayAccessInst{{newlvarname}, arrayname, indexname});
  mirgen.emplace(mir::AssignInst{{arrayaccessname}, rvar, arracctype}, types::Value(arracctype));
  return std::pair("", types::Void{});
}
lvarid AssignKnormVisitor::operator()(ast::TupleLvar& ast) {
  // how to get tuple element... add gettupleelement instruction?
  int count = 0;
  auto [rvar, rvartype] = std::visit(mirgen.exprvisitor, *expr);
  for (auto&& arg : ast.args) {
    mirgen.emplace(mir::FieldInst{{arg.value}, rvar, std::to_string(count++)}, rvartype);
  }
  return std::pair("", types::Void{});
}

lvarid StatementKnormVisitor::operator()(ast::Assign& ast) {
  auto visitor = AssignKnormVisitor(mirgen, ast.expr);
  return std::visit(visitor, ast.lvar);
}
lvarid StatementKnormVisitor::operator()(ast::Return& ast) {
  auto [ret, type] = std::visit(mirgen.exprvisitor, *ast.value);
  return mirgen.emplace(mir::ReturnInst{{mirgen.makeNewName()}, ret}, types::Value(type));
}
// Instructions StatementKnormVisitor::operator()(ast::Declaration& ast){}
lvarid StatementKnormVisitor::operator()(ast::For& /*ast*/) {
  // TODO(tomyoa)
  return lvarid{};
}

lvarid StatementKnormVisitor::operator()(ast::Fcall& ast) {
  return mirgen.genFcallInst(ast, std::nullopt);
}
lvarid StatementKnormVisitor::operator()(ast::Time& ast) {
  return mirgen.genFcallInst(ast.fcall, mirgen.genInst(ast.when).first);
}

}  // namespace mimium