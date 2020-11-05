/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/mirgenerator.hpp"

namespace mimium {

// new knormalizer(mir generator)

mir::blockptr MirGenerator::generate(ast::Statements& topast) {
  auto topblock = ast::Block{ast::DebugInfo{}, topast, std::nullopt};
  auto [optvalptr, ctx] = generateBlock(topblock, "root", std::nullopt);
  return ctx;
}

std::string MirGenerator::makeNewName() { return "k" + std::to_string(varcounter++); }

mir::valueptr MirGenerator::getOrGenExternalSymbol(std::string const& name,
                                                   types::Value const& type) {
  auto [iter, res] = external_symbols.try_emplace(
      name, std::make_shared<mir::Value>(mir::ExternalSymbol{name, type}));
  return iter->second;
}

mir::valueptr MirGenerator::require(optvalptr const& v) {
  if (auto res = v.value_or(nullptr)) { return res; }
  throw std::runtime_error("mir generation error: reference to value does not exist");
}
optvalptr MirGenerator::tryGetInternalSymbol(std::string const& name) {
  auto iter = symbol_table.find(name);
  return (iter != symbol_table.cend()) ? std::optional(iter->second) : std::nullopt;
}
mir::valueptr MirGenerator::getInternalSymbol(std::string const& name) {
  auto res = tryGetInternalSymbol(name);
  if (!res.has_value()) {
    throw std::runtime_error(" mir generation error: failed to resolve symbol name" + name);
  }
  return res.value();
}

std::pair<optvalptr, mir::blockptr> MirGenerator::generateBlock(ast::Block& block,
                                                                std::string label,
                                                                optvalptr const& fnctx) {
  auto tmpctx = this->block_ctx;
  this->fnctx = fnctx;
  auto blockctx = mir::makeBlock(label, indent_counter++);
  this->block_ctx = blockctx;
  auto retptr = exprvisitor(block);
  this->block_ctx = tmpctx;
  auto ret = (retptr == nullptr) ? std::nullopt : std::optional(retptr);
  indent_counter--;
  return {ret, blockctx};
}

bool MirGenerator::isPassByValue(types::Value const& type) { return types::isPrimitive(type); }

mir::valueptr MirGenerator::genAllocate(std::string const& name, types::Value const& type) {
  if (!isPassByValue(type)) {
    auto reftype = types::Pointer{type};
    return emplace(mir::instruction::Allocate{{name, reftype}, reftype});
  }
  return emplace(mir::instruction::Allocate{{name, type}, type});
}

using ExprKnormVisitor = MirGenerator::ExprKnormVisitor;
using StatementKnormVisitor = MirGenerator::StatementKnormVisitor;
using AssignKnormVisitor = MirGenerator::AssignKnormVisitor;
namespace minst = mir::instruction;

mir::valueptr ExprKnormVisitor::operator()(ast::Op& ast) {
  auto newname = mirgen.makeNewName();
  auto newlhs = ast.lhs.has_value() ? std::optional(genInst(ast.lhs.value())) : std::nullopt;
  auto newrhs = genInst(ast.rhs);
  return mirgen.emplace(
      minst::Op{{newname, types::Float{}}, ast.op, newlhs, mirgen.require(newrhs)});
}
mir::valueptr ExprKnormVisitor::operator()(ast::Number& ast) {
  return mirgen.emplace(minst::Number{{mirgen.makeNewName(), types::Float{}}, ast.value});
}
mir::valueptr ExprKnormVisitor::operator()(ast::String& ast) {
  return mirgen.emplace(minst::String{{mirgen.makeNewName(), types::String{}}, ast.value});
}
mir::valueptr ExprKnormVisitor::operator()(ast::Symbol& ast) {
  if (ast.value == "now") {  // todo: handle external symbols other than functions?
    return mirgen.emplace(
        minst::Fcall{{mirgen.makeNewName(), types::Float{}},
                     mirgen.getOrGenExternalSymbol("mimium_getnow", types::Float{}),
                     {},
                     FCALLTYPE::DIRECT,
                     std::nullopt});
  }

  if (auto ptrtoload = mirgen.tryGetInternalSymbol(ast.value + "_ptr")) {
    return mirgen.emplace(
        minst::Load{{mirgen.makeNewName(), mir::getType(*ptrtoload.value())}, ptrtoload.value()});
  }
  if (LLVMBuiltin::isBuiltin(ast.value)) {
    return mirgen.getOrGenExternalSymbol(ast.value,
                                         LLVMBuiltin::ftable.find(ast.value)->second.mmmtype);
  }
  throw std::runtime_error("symbol " + ast.value + " not found");
}  // namespace mimium
mir::valueptr ExprKnormVisitor::operator()(ast::Self& ast) {
  // todo: create special type for self
  MMMASSERT(mirgen.fnctx.has_value(), "Self cannot used in global context");
  auto self = mir::Self{mirgen.fnctx.value(), types::Float{}};
  return std::make_shared<mir::Value>(std::move(self));
}
mir::valueptr ExprKnormVisitor::operator()(ast::Lambda& ast) {
  auto label = mirgen.makeNewName();
  auto fun = minst::Function{
      {label, types::None{}},
      mirgen.transformArgs(ast.args.args, std::vector<std::shared_ptr<mir::Argument>>{},
                           [&](ast::DeclVar& lvar) {
                             auto& name = lvar.value.value;
                             auto& type = mirgen.typeenv.find(name);
                             auto res = std::make_shared<mir::Argument>(mir::Argument{name, type});
                             mirgen.symbol_table.emplace(name, std::make_shared<mir::Value>(*res));
                             return res;
                           }),
      {}};
  auto resptr = mirgen.emplace(std::move(fun));
  auto [blockret, ctx] = mirgen.generateBlock(ast.body, label, resptr);
  auto rettype = blockret ? getType(*blockret.value()) : types::Void{};
  auto& fref = mir::getInstRef<minst::Function>(resptr);
  auto* typeptr = mirgen.typeenv.tryFind(label);  // todo!!
  fref.body = ctx;
  fref.type = (typeptr != nullptr)
                  ? *typeptr
                  : types::Function{rettype, mirgen.transformArgs(
                                                 ast.args.args, std::vector<types::Value>{},
                                                 [&](ast::DeclVar& lvar) {
                                                   return mirgen.typeenv.find(lvar.value.value);
                                                 })};
  return resptr;
}
mir::valueptr MirGenerator::genFcallInst(ast::Fcall& fcall, optvalptr const& when) {
  auto fnptr = genInst(fcall.fn);
  types::Value rettype = mir::getType(*fnptr);
  auto fnkind = mir::isConstant(*fnptr) ? EXTERNAL : CLOSURE;
  auto newname = makeNewName();
  auto newargs = transformArgs(fcall.args.args, std::vector<mir::valueptr>{},
                               [&](auto expr) { return require(genInst(expr)); });
  return emplace(minst::Fcall{{newname, rettype}, fnptr, newargs, fnkind, when});
}
mir::valueptr ExprKnormVisitor::operator()(ast::Fcall& ast) {
  return mirgen.genFcallInst(ast, std::nullopt);
}

mir::valueptr ExprKnormVisitor::operator()(ast::Struct& /*ast*/) {
  // TODO(tomoya)
  return nullptr;
}
mir::valueptr ExprKnormVisitor::operator()(ast::StructAccess& /*ast*/) {
  // TODO(tomoya)
  return nullptr;
}
mir::valueptr ExprKnormVisitor::operator()(ast::ArrayInit& ast) {
  std::vector<mir::valueptr> newelems;
  types::Value lasttype;
  std::transform(ast.args.begin(), ast.args.end(), std::back_inserter(newelems),
                 [&](ast::ExprPtr e) { return genInst(e); });
  auto newname = mirgen.makeNewName();
  auto type = types::Array{mir::getType(*newelems[0])};
  return mirgen.emplace(minst::Array{{newname, type}, newelems});
}
mir::valueptr ExprKnormVisitor::operator()(ast::ArrayAccess& ast) {
  auto array = genInst(ast.array);
  types::Value rettype;
  if (std::holds_alternative<types::rArray>(mir::getType(*array))) {
    rettype = std::get<types::rArray>(mir::getType(*array)).getraw().elem_type;
  } else {
    throw std::runtime_error("[] operator cannot be used for other than array type");
  }
  auto newname = mirgen.makeNewName();
  auto index = genInst(ast.index);
  return mirgen.emplace(minst::Field{{newname, rettype}, array, index});
}
mir::valueptr ExprKnormVisitor::operator()(ast::Tuple& ast) {
  auto lvname = mirgen.makeNewName();
  std::vector<mir::valueptr> newelems;
  std::vector<types::Value> tupletypes;
  for (auto& a : ast.args) {
    auto arg = genInst(a);
    newelems.emplace_back(arg);
    tupletypes.emplace_back(mir::getType(*arg));
  }
  types::Value rettype = types::Tuple{{tupletypes}};
  mir::valueptr lvar = mirgen.emplace(minst::Allocate{{lvname + "_ref", rettype}, rettype});
  mirgen.symbol_table.emplace(lvname + "_ref", lvar);

  int count = 0;
  for (auto& elem : newelems) {
    auto newlvname = mirgen.makeNewName();
    auto index = std::make_shared<mir::Value>(mir::Constants{(double)count});
    auto ptrtostore =
        mirgen.emplace(minst::Field{{newlvname, tupletypes[count]}, lvar, std::move(index)});
    mirgen.emplace(minst::Store{{newlvname, tupletypes[count]}, ptrtostore, elem});
    count++;
  }
  return lvar;
}

mir::valueptr ExprKnormVisitor::operator()(ast::Block& ast) {
  for (auto& s : ast.stmts) { mirgen.genInst(*s); }
  if (ast.expr.has_value()) {
    auto val = genInst(ast.expr.value());
    return mirgen.emplace(minst::Return{{mirgen.makeNewName(), mir::getType(*val)}, val});
  }
  return nullptr;
}

std::pair<optvalptr, mir::blockptr> MirGenerator::genIfBlock(ast::ExprPtr& block,
                                                             std::string const& label) {
  auto realblock = (rv::holds_alternative<ast::Block>(*block))
                       ? rv::get<ast::Block>(*block)
                       : ast::Block{ast::DebugInfo{}, {}, block};
  return generateBlock(realblock, label, this->fnctx);
}

optvalptr MirGenerator::genIfInst(ast::If& ast, bool is_expr) {
  auto lvname = makeNewName();
  auto cond = genInst(ast.cond);
  auto [thenretval, thenblock] = genIfBlock(ast.then_stmts, lvname + "$then");
  auto rettype = mir::getType(*require(thenretval));
  if (ast.else_stmts.has_value()) {
    auto [elseretval, elseblock] = genIfBlock(ast.else_stmts.value(), lvname + "$else");
    return emplace(minst::If{{lvname, rettype}, cond, thenblock, elseblock});
  }
  return emplace(minst::If{{lvname, rettype}, cond, thenblock, std::nullopt});
}

mir::valueptr ExprKnormVisitor::operator()(ast::If& ast) {
  return mirgen.require(mirgen.genIfInst(ast, true));
}

void StatementKnormVisitor::operator()(ast::If& ast) { mirgen.genIfInst(ast, false); }
void StatementKnormVisitor::operator()(ast::Fdef& ast) {
  auto funexpr = ast::makeExpr(ast.fun);
  AssignKnormVisitor(mirgen, funexpr)(ast.lvar);
}

void AssignKnormVisitor::operator()(ast::DeclVar& ast) {
  auto& lvname = ast.value.value;
  optvalptr lvarptr = mirgen.tryGetInternalSymbol(lvname);
  types::Value& type = mirgen.typeenv.find(lvname);
  mir::valueptr ptr;
  if (lvarptr.has_value()) {
    MMMASSERT(type == mir::getType(*require(lvarptr)), "lvar type are different")
    ptr = lvarptr.value();
  } else {
    ptr = mirgen.genAllocate(lvname, type);
  }
  mirgen.symbol_table.emplace(lvname + "_ptr", ptr);
  auto rvar = mirgen.genInst(expr);
  mirgen.emplace(minst::Store{{lvname, types::None{}}, ptr, rvar});
}
void AssignKnormVisitor::operator()(ast::ArrayLvar& ast) {
  auto array = mirgen.genInst(ast.array);
  auto index = mirgen.genInst(ast.index);
  auto rvar = mirgen.genInst(expr);
  auto name = mir::getName(*array) + mir::getName(*index);
  auto type = mir::getType(*rvar);
  auto ptrtostore = mirgen.emplace(minst::Field{{name, type}, array, index});
  mirgen.emplace(minst::Store{{name, type}, ptrtostore, rvar});
}  // namespace mimium
void AssignKnormVisitor::operator()(ast::TupleLvar& ast) {
  int count = 0;
  auto rvar = mirgen.genInst(expr);
  for (auto&& arg : ast.args) {
    auto& name = arg.value.value;
    auto& type = mirgen.typeenv.find(name);
    mir::valueptr lvar = mirgen.emplace(minst::Allocate{{name + "_ptr", type}, type});
    mirgen.symbol_table.emplace(name + "_ptr", lvar);

    mir::Constants index = count++;
    auto ptrtoval = mirgen.emplace(
        minst::Field{{name, type}, rvar, std::make_shared<mir::Value>(std::move(index))});
    auto valtostore = mirgen.emplace(minst::Load{{mirgen.makeNewName(), type}, ptrtoval});
    mirgen.emplace(minst::Store{{name, type}, lvar, valtostore});
  }
}

void StatementKnormVisitor::operator()(ast::Assign& ast) {
  auto visitor = AssignKnormVisitor(mirgen, ast.expr);
  std::visit(visitor, ast.lvar);
}
void StatementKnormVisitor::operator()(ast::Return& ast) {
  auto val = mirgen.genInst(ast.value);
  mirgen.emplace(minst::Return{{mirgen.makeNewName(), mir::getType(*val)}, val});
}
// Instructions StatementKnormVisitor::operator()(ast::Declaration& ast){}
void StatementKnormVisitor::operator()(ast::For& /*ast*/) {
  // TODO(tomyoa)
}

void StatementKnormVisitor::operator()(ast::Fcall& ast) { mirgen.genFcallInst(ast, std::nullopt); }
void StatementKnormVisitor::operator()(ast::Time& ast) {
  mirgen.genFcallInst(ast.fcall, mirgen.genInst(ast.when));
}

}  // namespace mimium