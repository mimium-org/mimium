/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/mirgenerator.hpp"

namespace mimium {

// new knormalizer(mir generator)

std::shared_ptr<MIRblock> MirGenerator::generate(ast::Statements& topast) {
  auto topblock = ast::Block{ast::DebugInfo{}, topast, std::nullopt};
  auto [lvarid, ctx] = generateBlock(topblock, "root");
  return ctx;
}

std::string MirGenerator::makeNewName() {
  auto res = lvar_holder.value_or("k" + std::to_string(varcounter++));
  lvar_holder = std::nullopt;
  return res;
}

std::pair<lvarid, std::shared_ptr<MIRblock>> MirGenerator::generateBlock(
    ast::Block& block, std::string label) {
  int indent = (ctx == nullptr) ? 0 : (ctx->indent_level + 1);
  auto tmpctx = ctx;
  auto functx = std::make_shared<MIRblock>(label);
  ctx = functx;
  ctx->indent_level = indent;
  lvarid laststmt;
  for (auto&& s : block.stmts) {
    laststmt = genInst(*s);
  }
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

lvarid ExprKnormVisitor::operator()(ast::Op& ast) {
  auto newname = mirgen.makeNewName();
  auto lhsname =
      ast.lhs.has_value() ? std::visit(*this, *ast.lhs.value()).first : "";
  auto rhsname = std::visit(*this, *ast.rhs).first;
  return mirgen.emplace(OpInst(newname, ast.op, lhsname, rhsname),
                        types::Float());
}
lvarid ExprKnormVisitor::operator()(ast::Number& ast) {
  return mirgen.emplace(NumberInst(mirgen.makeNewName(), ast.value),
                        types::Float());
}
lvarid ExprKnormVisitor::operator()(ast::String& ast) {
  return mirgen.emplace(StringInst(mirgen.makeNewName(), ast.value),
                        types::String());
}
lvarid ExprKnormVisitor::operator()(ast::Rvar& ast) {
  if (ast.value == "now") {
    return mirgen.emplace(
        FcallInst(mirgen.makeNewName(), "mimium_getnow", {}, DIRECT),
        types::Float());
  }
  return std::pair(ast.value, mirgen.typeenv.find(ast.value));
}
lvarid ExprKnormVisitor::operator()(ast::Self& ast) {
  lvarid res = {"self", ast.type.value_or(types::Float())};
  return res;
}
lvarid ExprKnormVisitor::operator()(ast::Lambda& ast) {
  auto label = mirgen.makeNewName();

  auto fun = FunInst(
      label, mirgen.transformArgs(ast.args.args, std::deque<std::string>{},
                                  [&](ast::Lvar& lvar) { return lvar.value; }));
  auto* typeptr = mirgen.typeenv.tryFind(label);
  auto [lvar, ctx] = mirgen.generateBlock(ast.body, label);
  fun.body = ctx;
  types::Value type =
      (typeptr != nullptr)
          ? *typeptr
          : types::Function(
                lvar.second,
                mirgen.transformArgs(ast.args.args, std::vector<types::Value>{},
                                     [&](ast::Lvar& lvar) {
                                       return mirgen.typeenv.find(lvar.value);
                                     }));
  return mirgen.emplace(std::move(fun), std::move(type));
}
lvarid MirGenerator::genFcallInst(ast::Fcall& fcall,
                                  std::optional<std::string> when) {
  auto [fname, type] = genInst(fcall.fn);
  auto* rettype_ptr = std::get_if<types::rFunction>(&typeenv.find(fname));
  types::Value rettype =
      (rettype_ptr == nullptr) ? types::None() : rettype_ptr->getraw().ret_type;
  auto fnkind = MirGenerator::isExternalFun(fname) ? EXTERNAL : CLOSURE;
  auto newname = makeNewName();
  auto newargs = transformArgs(fcall.args.args, std::deque<std::string>{},
                               [&](auto expr) { return genInst(expr).first; });
  return emplace(FcallInst(newname, fname, newargs, fnkind, rettype, when),
                 rettype);
}
lvarid ExprKnormVisitor::operator()(ast::Fcall& ast,
                                    std::optional<std::string> when) {
  return mirgen.genFcallInst(ast, when);
}

lvarid ExprKnormVisitor::operator()(ast::Struct& ast) {
  // TODO
  return lvarid{};
}
lvarid ExprKnormVisitor::operator()(ast::StructAccess& ast) {
  // TODO
  return lvarid{};
}
lvarid ExprKnormVisitor::operator()(ast::ArrayInit& ast) {
  // TODO
  return lvarid{};
}
lvarid ExprKnormVisitor::operator()(ast::ArrayAccess& ast) {
  auto [arrname, arrtype] = std::visit(*this, *ast.array);
  auto* arrtype_ptr = std::get_if<types::rArray>(&arrtype);
  types::Value rettype;
  if (arrtype_ptr != nullptr) {
    rettype = arrtype_ptr->getraw().elem_type;
  } else {
    throw std::runtime_error(
        "[] operator cannot be used for other than array type");
  }
  auto newname = mirgen.makeNewName();
  return mirgen.emplace(
      ArrayAccessInst(newname, arrname, std::visit(*this, *ast.index).first),
      std::move(rettype));
}
lvarid ExprKnormVisitor::operator()(ast::Tuple& ast) {
  // TODO
  return lvarid{};
}

lvarid ExprKnormVisitor::operator()(ast::Block& ast) {
  static_assert(true, "should be unreachable");
  return lvarid{};
}

std::pair<lvarid,std::shared_ptr<MIRblock>> MirGenerator::genIfBlock(ast::ExprPtr& block,std::string const& label) {
  auto realblock = (rv::holds_alternative<ast::Block>(*block))? rv::get<ast::Block>(*block): ast::Block{ast::DebugInfo{},{},block};
  return generateBlock(realblock, label);
}


lvarid MirGenerator::genIfInst(ast::If& ast) {
  auto lvname = makeNewName();
  auto ifinst = IfInst(lvname, genInst(ast.cond).first);
  auto [thenvarid, thenblock] = genIfBlock(ast.then_stmts, lvname + "$then");
  ifinst.thenblock = thenblock;
  ifinst.elseblock =
      ast.else_stmts.has_value()
          ? genIfBlock(ast.else_stmts.value(), lvname + "$else").second
          : nullptr;
  return emplace(std::move(ifinst), types::Value(thenvarid.second));
}

lvarid ExprKnormVisitor::operator()(ast::If& ast) {
  return mirgen.genIfInst(ast);
}
lvarid StatementKnormVisitor::operator()(ast::If& ast) {
  return mirgen.genIfInst(ast);
}
lvarid StatementKnormVisitor::operator()(ast::Fdef& ast) {
  bool is_overwrite = mirgen.isOverWrite(ast.lvar.value);
  if (!is_overwrite) {
    mirgen.lvar_holder = ast.lvar.value;
  }
  return mirgen.exprvisitor(ast.fun);
}

lvarid StatementKnormVisitor::operator()(ast::Assign& ast) {
  bool is_overwrite = mirgen.isOverWrite(ast.lvar.value);
  if (!is_overwrite) {
    mirgen.lvar_holder = ast.lvar.value;
  }
  lvarid res = std::visit(mirgen.exprvisitor, *ast.expr);
  if (!rv::holds_alternative<types::Function>(res.second)) {
    if (!is_overwrite) {
      std::string ptrname = ast.lvar.value;
      types::Value ptrtype = res.second;
      auto iter = mirgen.ctx->instructions.insert(
          --mirgen.ctx->instructions.end(), AllocaInst(ptrname, ptrtype));
      // mirgen.typeenv.emplace(ptrname, ptrtype);
      mirgen.lvarlist.emplace_back(ast.lvar.value);
    }

    if (is_overwrite) {
      res = mirgen.emplace(AssignInst(ast.lvar.value, res.first, res.second),
                           types::Value(res.second));
    }
  }
  return res;
}
lvarid StatementKnormVisitor::operator()(ast::Return& ast) {
  auto [ret, type] = std::visit(mirgen.exprvisitor, *ast.value);
  return mirgen.emplace(ReturnInst(mirgen.makeNewName(), ret, type),
                        types::Value(type));
}
// Instructions StatementKnormVisitor::operator()(ast::Declaration& ast){}
lvarid StatementKnormVisitor::operator()(ast::For& ast) {
  // TODO
  return lvarid{};
}

lvarid StatementKnormVisitor::operator()(ast::Fcall& ast) {
  return mirgen.genFcallInst(ast, std::nullopt);
}
lvarid StatementKnormVisitor::operator()(ast::Time& ast) {
  return mirgen.genFcallInst(ast.fcall, mirgen.genInst(ast.when).first);
}

}  // namespace mimium