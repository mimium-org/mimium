/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/mirgenerator.hpp"

namespace mimium {

// new knormalizer(mir generator)

mir::blockptr MirGenerator::generate(ast::Statements& topast) {
  auto topblock = ast::Block{ast::DebugInfo{}, topast, std::nullopt};
  auto [optvalptr, ctx] = generateBlock(topblock, "root");
  return ctx;
}

std::string MirGenerator::makeNewName() { return "k" + std::to_string(varcounter++); }

mir::valueptr MirGenerator::getOrGenExternalSymbol(std::string const& name,
                                                   types::Value const& type) {
  auto [iter, res] = external_symbols.try_emplace(name, mir::Value{name, type});
  return &iter->second;
}

mir::valueptr MirGenerator::require(optvalptr const& v) {
  if (auto* res = v.value_or(nullptr)) { return res; }
  throw std::runtime_error("mir generation error: reference to value does not exist");
}
optvalptr MirGenerator::tryGetInternalSymbol(std::string const& name) {
  auto iter = symbol_table.find(name);
  return (iter != symbol_table.cend()) ? std::optional(&iter->second) : std::nullopt;
}
mir::valueptr MirGenerator::getInternalSymbol(std::string const& name) {
  auto res = tryGetInternalSymbol(name);
  if (res.has_value()) {
    throw std::runtime_error(" mir generation error: failed to resolve symbol name" + name);
  }
  return res.value();
}

std::pair<optvalptr, mir::blockptr> MirGenerator::generateBlock(ast::Block& block,
                                                                std::string label) {
  int indent = (ctx == nullptr) ? 0 : (ctx->indent_level + 1);
  auto tmpctx = ctx;
  auto functx = mir::makeBlock(label, indent);
  ctx = functx;
  ctx->indent_level = indent;
  optvalptr laststmt;
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
namespace minst = mir::instruction;

optvalptr ExprKnormVisitor::operator()(ast::Op& ast) {
  auto newname = mirgen.makeNewName();
  auto newlhs = ast.lhs.has_value() ? genInst(ast.lhs.value()) : std::nullopt;
  auto newrhs = genInst(ast.rhs);
  return mirgen.emplace(
      minst::Op{{mir::Value{newname, types::Float{}}}, ast.op, newlhs, mirgen.require(newrhs)});
}
optvalptr ExprKnormVisitor::operator()(ast::Number& ast) {
  return mirgen.emplace(
      minst::Number{{mir::Value{mirgen.makeNewName(), types::Float{}}}, ast.value});
}
optvalptr ExprKnormVisitor::operator()(ast::String& ast) {
  return mirgen.emplace(
      minst::String{{mir::Value{mirgen.makeNewName(), types::String{}}}, ast.value});
}
optvalptr ExprKnormVisitor::operator()(ast::Symbol& ast) {
  if (ast.value == "now") {  // todo: handle external symbols other than functions?
    return mirgen.emplace(
        minst::Fcall{{mir::Value{mirgen.makeNewName(), types::Float{}}},
                     mirgen.getOrGenExternalSymbol("mimium_getnow", types::Float{}),
                     {},
                     FCALLTYPE::DIRECT,
                     std::nullopt});
  }
  auto* ptrtoload = mirgen.require(mirgen.getInternalSymbol(ast.value + "_ptr"));
  return mirgen.emplace(
      minst::Load{{mir::Value{mirgen.makeNewName(), ptrtoload->type}}, ptrtoload});

}  // namespace mimium
optvalptr ExprKnormVisitor::operator()(ast::Self& ast) {
  // todo: create special type for self
  return mirgen.getOrGenExternalSymbol("self", types::Float{});
}
optvalptr ExprKnormVisitor::operator()(ast::Lambda& ast) {
  auto label = mirgen.makeNewName();
  auto fun = minst::Function{
      {mir::Value{label, types::None{}}},
      mirgen.transformArgs(ast.args.args, std::deque<mir::valueptr>{},
                           [&](ast::DeclVar& lvar) {
                             auto& name = lvar.value.value;
                             auto& type = mirgen.typeenv.find(name);
                             return mirgen.symbol_table.emplace(name, mir::Value{name, type});
                           }),
      {}};
  auto* typeptr = mirgen.typeenv.tryFind(label);
  auto [rettype, ctx] = mirgen.generateBlock(ast.body, label);
  fun.body = ctx;
  fun.lv_name->type =
      (typeptr != nullptr)
          ? *typeptr
          : types::Function{mirgen.require(rettype)->type,
                            mirgen.transformArgs(ast.args.args, std::vector<types::Value>{},
                                                 [&](ast::DeclVar& lvar) {
                                                   return mirgen.typeenv.find(lvar.value.value);
                                                 })};
  return mirgen.emplace(std::move(fun));
}
optvalptr MirGenerator::genFcallInst(ast::Fcall& fcall, optvalptr const& when) {
  auto fun = this->require(genInst(fcall.fn));
  auto* rettype_ptr = std::get_if<types::rFunction>(&typeenv.find(fun->name));
  types::Value rettype = (rettype_ptr == nullptr) ? types::None() : rettype_ptr->getraw().ret_type;
  auto fnkind = MirGenerator::isExternalFun(fun->name) ? EXTERNAL : CLOSURE;
  auto newname = makeNewName();
  auto newargs = transformArgs(fcall.args.args, std::deque<mir::valueptr>{},
                               [&](auto expr) { return require(genInst(expr)); });
  return emplace(minst::Fcall{{mir::Value{newname, rettype}}, fun, newargs, fnkind, when});
}
optvalptr ExprKnormVisitor::operator()(ast::Fcall& ast) {
  return mirgen.genFcallInst(ast, std::nullopt);
}

optvalptr ExprKnormVisitor::operator()(ast::Struct& /*ast*/) {
  // TODO(tomoya)
  return optvalptr{};
}
optvalptr ExprKnormVisitor::operator()(ast::StructAccess& /*ast*/) {
  // TODO(tomoya)
  return optvalptr{};
}
optvalptr ExprKnormVisitor::operator()(ast::ArrayInit& ast) {
  std::deque<mir::valueptr> newelems;
  types::Value lasttype;
  std::transform(ast.args.begin(), ast.args.end(), std::back_inserter(newelems),
                 [&](ast::ExprPtr e) { return mirgen.require(genInst(e)); });
  auto newname = mirgen.makeNewName();
  auto type = types::Array{newelems[0]->type};
  return mirgen.emplace(minst::Array{{mir::Value{newname, type}}, newelems});
}
optvalptr ExprKnormVisitor::operator()(ast::ArrayAccess& ast) {
  auto array = mirgen.require(genInst(ast.array));
  auto* arrtype_ptr = std::get_if<types::rArray>(&array->type);
  types::Value rettype;
  if (arrtype_ptr != nullptr) {
    rettype = arrtype_ptr->getraw().elem_type;
  } else {
    throw std::runtime_error("[] operator cannot be used for other than array type");
  }
  auto newname = mirgen.makeNewName();
  auto index = mirgen.require(genInst(ast.index));
  return mirgen.emplace(minst::Field{{mir::Value{newname, rettype}}, array, index});
}
optvalptr ExprKnormVisitor::operator()(ast::Tuple& ast) {
  auto lvname = mirgen.makeNewName();
  std::vector<mir::valueptr> newelems;
  std::vector<types::Value> tupletypes;
  for (auto& a : ast.args) {
    auto* arg = mirgen.require(genInst(a));
    newelems.emplace_back(arg);
    tupletypes.emplace_back(arg->type);
  }
  types::Value rettype = types::Tuple{{tupletypes}};
  mir::valueptr lvar =
      mirgen.require(mirgen.emplace(minst::Allocate{{mir::Value{lvname + "_ptr", rettype}}}));
  int count = 0;
  for (auto& elem : newelems) {
    auto newlvname = mirgen.makeNewName();
    auto* ptrtostore = mirgen.require(
        mirgen.emplace(minst::Field{{mir::Value{newlvname, tupletypes[count]}}, lvar, count}));
    mirgen.emplace(minst::Store{{mir::Value{newlvname, tupletypes[count]}}, ptrtostore, elem});
    count++;
  }
  return lvar;
}

optvalptr ExprKnormVisitor::operator()(ast::Block& /*ast*/) {
  static_assert(true, "should be unreachable");
  return optvalptr{};
}

std::pair<optvalptr, mir::blockptr> MirGenerator::genIfBlock(ast::ExprPtr& block,
                                                             std::string const& label) {
  auto realblock = (rv::holds_alternative<ast::Block>(*block))
                       ? rv::get<ast::Block>(*block)
                       : ast::Block{ast::DebugInfo{}, {}, block};
  return generateBlock(realblock, label);
}

optvalptr MirGenerator::genIfInst(ast::If& ast, bool need_alloca) {
  auto lvname = makeNewName();
  auto* cond = this->require(genInst(ast.cond));
  auto [thenretval, thenblock] = genIfBlock(ast.then_stmts, lvname + "$then");
  types::Value& rettype = this->require(thenretval)->type;
  std::optional<mir::blockptr> elseblock =
      ast.else_stmts.has_value()
          ? std::optional(genIfBlock(ast.else_stmts.value(), lvname + "$else").second)
          : std::nullopt;
  if (need_alloca) { emplace(minst::Allocate{{mir::Value{lvname, rettype}}, rettype}); }
  return emplace(minst::If{{mir::Value{lvname, rettype}}, cond, thenblock, elseblock});
}

optvalptr ExprKnormVisitor::operator()(ast::If& ast) { return mirgen.genIfInst(ast, true); }

optvalptr StatementKnormVisitor::operator()(ast::If& ast) { return mirgen.genIfInst(ast, false); }
optvalptr StatementKnormVisitor::operator()(ast::Fdef& ast) {
  auto funexpr = ast::makeExpr(ast.fun);
  AssignKnormVisitor(mirgen, funexpr)(ast.lvar);
  return std::nullopt;
}

optvalptr AssignKnormVisitor::operator()(ast::DeclVar& ast) {
  auto& lvname = ast.value.value;
  optvalptr lvarptr = mirgen.tryGetInternalSymbol(lvname);
  auto& type = mirgen.typeenv.find(lvname);
  mir::valueptr ptr = lvarptr.value_or(
      mirgen.require(mirgen.emplace(minst::Allocate{{mir::Value{lvname + "_ptr", type}}})));
  mirgen.symbol_table.emplace(lvname + "_ptr", ptr);
  auto* rvar = mirgen.require(mirgen.genInst(expr));
  mirgen.emplace(minst::Store{{std::nullopt}, ptr, rvar});
  return std::nullopt;
}
optvalptr AssignKnormVisitor::operator()(ast::ArrayLvar& ast) {
  auto* array = mirgen.require(mirgen.genInst(ast.array));
  auto* index = mirgen.require(mirgen.genInst(ast.index));
  auto* rvar = mirgen.require(mirgen.genInst(expr));
  auto* ptrtostore = mirgen.require(mirgen.emplace(
      minst::Field{{mir::Value{array->name + index->name, rvar->type}}, array, index}));
  mirgen.emplace(minst::Store{{std::nullopt}, ptrtostore, rvar});
  return std::nullopt;
}
optvalptr AssignKnormVisitor::operator()(ast::TupleLvar& ast) {
  // how to get tuple element... add gettupleelement instruction?
  int count = 0;
  auto* rvar = mirgen.require(mirgen.genInst(expr));
  for (auto&& arg : ast.args) {
    auto& name = arg.value.value;

    auto& type = mirgen.typeenv.find(name);
    mir::valueptr lvar =
        mirgen.require(mirgen.emplace(minst::Allocate{{mir::Value{name + "_ptr", type}}}));
    auto* ptrtoval =
        mirgen.require(mirgen.emplace(minst::Field{{mir::Value{name, type}}, rvar, count++}));
    auto* valtostore = mirgen.require(
        mirgen.emplace(minst::Load{{mir::Value{mirgen.makeNewName(), type}}, ptrtoval}));
    mirgen.emplace(minst::Store{{std::nullopt}, lvar, valtostore});
  }
  return std::nullopt;
}

optvalptr StatementKnormVisitor::operator()(ast::Assign& ast) {
  auto visitor = AssignKnormVisitor(mirgen, ast.expr);
  return std::visit(visitor, ast.lvar);
}
optvalptr StatementKnormVisitor::operator()(ast::Return& ast) {
  auto* val = mirgen.require(mirgen.genInst(ast.value));
  return mirgen.emplace(minst::Return{{mir::Value{mirgen.makeNewName(), val->type}}, val});
}
// Instructions StatementKnormVisitor::operator()(ast::Declaration& ast){}
optvalptr StatementKnormVisitor::operator()(ast::For& /*ast*/) {
  // TODO(tomyoa)
  return optvalptr{};
}

optvalptr StatementKnormVisitor::operator()(ast::Fcall& ast) {
  return mirgen.genFcallInst(ast, std::nullopt);
}
optvalptr StatementKnormVisitor::operator()(ast::Time& ast) {
  return mirgen.genFcallInst(ast.fcall, mirgen.genInst(ast.when));
}

}  // namespace mimium