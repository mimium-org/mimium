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
mir::valueptr MirGenerator::getFunctionSymbol(std::string const& name, types::Value const& type) {
  auto res = tryGetInternalSymbol(name);
  return res.value_or(getOrGenExternalSymbol(name, type));
}
mir::valueptr MirGenerator::getInternalSymbol(std::string const& name) {
  auto res = tryGetInternalSymbol(name);
  if (!res.has_value()) {
    throw std::runtime_error(" mir generation error: failed to resolve symbol name " + name);
  }
  return res.value();
}

using ExprKnormVisitor = MirGenerator::ExprKnormVisitor;
using StatementKnormVisitor = MirGenerator::StatementKnormVisitor;
using AssignKnormVisitor = MirGenerator::AssignKnormVisitor;
namespace minst = mir::instruction;

std::pair<optvalptr, mir::blockptr> MirGenerator::generateBlock(ast::Block& block,
                                                                std::string label,
                                                                optvalptr const& fnctx) {
  auto blockctx = mir::makeBlock(label, indent_counter++);
  blockctx->parent = fnctx;
  ExprKnormVisitor exprvisitor(*this, blockctx, fnctx);
  auto retptr = exprvisitor(block);
  auto ret = (retptr == nullptr) ? std::nullopt : std::optional(retptr);
  indent_counter--;
  return {ret, blockctx};
}

bool MirGenerator::isPassByValue(types::Value const& type) {
  // todo: array handling
  return !rv::holds_alternative<types::Tuple>(type);
}

mir::valueptr ExprKnormVisitor::emplace(mir::Instructions&& inst) {
  return mir::addInstToBlock(std::move(inst), this->block);
}

mir::valueptr ExprKnormVisitor::genAllocate(std::string const& name, types::Value const& type) {
  // if (!isPassByValue(type)) {
  //   auto reftype = types::Pointer{type};
  //   return emplace(mir::instruction::Allocate{{name, reftype}, reftype});
  // }
  return emplace(mir::instruction::Allocate{{name, type}});
}

mir::valueptr ExprKnormVisitor::operator()(ast::Op& ast) {
  auto newname = mirgen.makeNewName();
  auto newlhs = ast.lhs.has_value() ? std::optional(genInst(ast.lhs.value())) : std::nullopt;
  auto newrhs = genInst(ast.rhs);
  return emplace(minst::Op{{newname, types::Float{}}, ast.op, newlhs, mirgen.require(newrhs)});
}
mir::valueptr ExprKnormVisitor::operator()(ast::Number& ast) {
  return emplace(minst::Number{{mirgen.makeNewName(), types::Float{}}, ast.value});
}
mir::valueptr ExprKnormVisitor::operator()(ast::String& ast) {
  return emplace(minst::String{{mirgen.makeNewName(), types::String{}}, ast.value});
}
mir::valueptr ExprKnormVisitor::operator()(ast::Symbol& ast) {
  if (ast.value == "now") {  // todo: handle external symbols other than functions?
    return emplace(minst::Fcall{{mirgen.makeNewName(), types::Float{}},
                                mirgen.getOrGenExternalSymbol("mimium_getnow", types::Float{}),
                                {},
                                FCALLTYPE::DIRECT,
                                std::nullopt});
  }

  if (auto ptrtoload = mirgen.tryGetInternalSymbol(ast.value + "_ptr")) {
    return emplace(
        minst::Load{{mirgen.makeNewName(), mir::getType(*ptrtoload.value())}, ptrtoload.value()});
  }
  if (auto arg = mirgen.tryGetInternalSymbol(ast.value)) {
    bool is_argument = std::holds_alternative<std::shared_ptr<mir::Argument>>(*arg.value());
    bool is_function = mir::isInstA<minst::Function>(arg.value());
    MMMASSERT(is_argument || is_function,
              "failed to find symbol. Internal symbols should be a pointer for value, argument or "
              "function")
    return arg.value();
  }
  if (LLVMBuiltin::isBuiltin(ast.value)) {
    return mirgen.getOrGenExternalSymbol(ast.value,
                                         LLVMBuiltin::ftable.find(ast.value)->second.mmmtype);
  }
  throw std::runtime_error("symbol " + ast.value + " not found");
}  // namespace mimium
mir::valueptr ExprKnormVisitor::operator()(ast::Self& ast) {
  // todo: create special type for self
  MMMASSERT(fnctx.has_value(), "Self cannot used in global context");
  auto self = mir::Self{fnctx.value(), types::Float{}};
  return std::make_shared<mir::Value>(std::move(self));
}
mir::valueptr ExprKnormVisitor::operator()(ast::Lambda& ast) {
  auto argtypes = std::vector<types::Value>{};
  auto args = std::list<std::shared_ptr<mir::Argument>>{};
  auto label = lvar_holder.has_value() ? lvar_holder.value() : mirgen.makeNewName();
  auto fun = minst::Function{{label, types::None{}},
                             mirgen.transformArgs(ast.args.args, args, mirgen.make_arguments)};
  auto resptr = emplace(std::move(fun));
  auto [blockret, body] = mirgen.generateBlock(ast.body, label, resptr);
  auto rettype = blockret ? getType(*blockret.value()) : types::Void{};
  auto& fref = mir::getInstRef<minst::Function>(resptr);

  for (auto& a : fref.args) { a->parentfn = resptr; }

  fref.type = types::Function{
      rettype, mirgen.transformArgs(ast.args.args, argtypes, [&](ast::DeclVar& lvar) {
        return mirgen.typeenv.find(lvar.value.value);
      })};
  fref.body = body;
  return resptr;
}
mir::valueptr ExprKnormVisitor::genFcallInst(ast::Fcall& fcall, optvalptr const& when) {
  mir::valueptr fnptr = nullptr;
  bool is_fn_recursive = false;
  if (auto* fnlabel = std::get_if<ast::Symbol>(fcall.fn.get())) {
    auto& name = fnlabel->value;
    if (fnctx.has_value()) {
      auto& cur_fn = mir::getInstRef<minst::Function>(fnctx.value());
      cur_fn.isrecursive |= name == cur_fn.name;
      is_fn_recursive = cur_fn.isrecursive;
    }
    fnptr = is_fn_recursive ? this->fnctx.value()
                            : mirgen.getFunctionSymbol(name, mirgen.typeenv.find(name));
  } else {
    fnptr = genInst(fcall.fn);
  }
  auto newname = mirgen.makeNewName();
  bool is_fn_ext = std::holds_alternative<mir::ExternalSymbol>(*fnptr);
  auto fnkind = is_fn_ext && !is_fn_recursive ? EXTERNAL : CLOSURE;
  auto args = mirgen.transformArgs(fcall.args.args, std::list<mir::valueptr>{},
                                   [&](auto expr) { return genInst(expr); });
  types::Value rettype = types::None{};
  if (!is_fn_recursive) {
    auto ftype = mir::getType(*fnptr);
    rettype = rv::get<types::Function>(ftype).ret_type;
  }

  return emplace(minst::Fcall{{newname, rettype}, fnptr, args, fnkind, when});
}
mir::valueptr ExprKnormVisitor::operator()(ast::Fcall& ast) {
  return genFcallInst(ast, std::nullopt);
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
  return emplace(minst::Array{{newname, type}, newelems});
}
mir::valueptr ExprKnormVisitor::operator()(ast::ArrayAccess& ast) {
  auto array = genInst(ast.array);
  types::Value rettype;
  auto type = mir::getType(*array);
  if (std::holds_alternative<types::rArray>(type)) {
    rettype = rv::get<types::Array>(type).elem_type;
  } else if (std::holds_alternative<types::rPointer>(type)) {
    rettype = rv::get<types::Pointer>(type).val;
  } else {
    throw std::runtime_error("[] operator cannot be used for other than array type");
  }
  auto newname = mirgen.makeNewName();
  auto index = genInst(ast.index);
  return emplace(minst::ArrayAccess{{newname, rettype}, array, index});
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
  mir::valueptr lvar = genAllocate(lvname + "_ref", rettype);
  mirgen.symbol_table.emplace(lvname + "_ref", lvar);

  int count = 0;
  for (auto& elem : newelems) {
    auto newlvname = mirgen.makeNewName();
    auto index = std::make_shared<mir::Value>(mir::Constants{(double)count});
    auto ptrtostore = emplace(minst::Field{{newlvname, tupletypes[count]}, lvar, std::move(index)});
    emplace(minst::Store{{newlvname, tupletypes[count]}, ptrtostore, elem});
    count++;
  }
  // mir::valueptr res = emplace(minst::Load{{lvname, rettype}, lvar});

  return lvar;
}

mir::valueptr ExprKnormVisitor::operator()(ast::Block& ast) {
  StatementKnormVisitor svisitor(*this);
  for (auto& s : ast.stmts) { svisitor.genInst(*s); }
  if (ast.expr.has_value()) {
    auto val = genInst(ast.expr.value());
    return emplace(minst::Return{{mirgen.makeNewName(), mir::getType(*val)}, val});
  }
  return svisitor.retvalue.value_or(nullptr);
}

std::pair<optvalptr, mir::blockptr> ExprKnormVisitor::genIfBlock(ast::ExprPtr& block,
                                                                 std::string const& label) {
  auto realblock = rv::holds_alternative<ast::Block>(*block)
                       ? rv::get<ast::Block>(*block)
                       : ast::Block{ast::DebugInfo{}, {}, block};
  return mirgen.generateBlock(realblock, label, this->fnctx);
}

mir::valueptr ExprKnormVisitor::operator()(ast::If& ast) {
  auto lvname = mirgen.makeNewName();
  auto cond = genInst(ast.cond);
  auto [thenretval, thenblock] = genIfBlock(ast.then_stmts, lvname + "$then");
  auto rettype = mir::getType(*require(thenretval));
  if (ast.else_stmts.has_value()) {
    auto [elseretval, elseblock] = genIfBlock(ast.else_stmts.value(), lvname + "$else");
    return emplace(minst::If{{lvname, rettype}, cond, thenblock, elseblock});
  }
  return emplace(minst::If{{lvname, rettype}, cond, thenblock, std::nullopt});
}

void StatementKnormVisitor::operator()(ast::If& ast) { exprvisitor(ast); }
void StatementKnormVisitor::operator()(ast::Fdef& ast) {
  auto funexpr = ast::makeExpr(ast.fun);
  AssignKnormVisitor(mirgen, exprvisitor, funexpr)(ast.lvar);
}

void AssignKnormVisitor::operator()(ast::DeclVar& ast) {
  auto& lvname = ast.value.value;
  optvalptr lvarptr = mirgen.tryGetInternalSymbol(lvname + "_ptr");
  types::Value& type = mirgen.typeenv.find(lvname);
  mir::valueptr ptr;
  if (std::holds_alternative<types::rFunction>(type)) {
    // do not allocate and store for function definition
    auto rvar = exprvisitor.genInst(expr, lvname);
    mirgen.symbol_table.emplace(lvname, rvar);
    return;
  }
  if (lvarptr.has_value()) {
    MMMASSERT(type == mir::getType(*require(lvarptr)), "lvar type are different")
    ptr = lvarptr.value();
  } else {
    ptr = exprvisitor.genAllocate(lvname, type);
  }
  mirgen.symbol_table.emplace(lvname + "_ptr", ptr);
  auto rvar = exprvisitor.genInst(expr);

  exprvisitor.emplace(minst::Store{{lvname, types::None{}}, ptr, rvar});
}
void AssignKnormVisitor::operator()(ast::ArrayLvar& ast) {
  auto array = exprvisitor.genInst(ast.array);
  auto index = exprvisitor.genInst(ast.index);
  auto rvar = exprvisitor.genInst(expr);
  auto name = mir::getName(*array) + mir::getName(*index);
  auto type = mir::getType(*rvar);
  auto ptrtostore = exprvisitor.emplace(minst::Field{{name, type}, array, index});
  exprvisitor.emplace(minst::Store{{name, type}, ptrtostore, rvar});
}  // namespace mimium
void AssignKnormVisitor::operator()(ast::TupleLvar& ast) {
  int count = 0;
  auto rvar = exprvisitor.genInst(expr);
  for (auto&& arg : ast.args) {
    auto& name = arg.value.value;
    auto& type = mirgen.typeenv.find(name);
    mir::valueptr lvar = exprvisitor.genAllocate(name + "_ptr", type);
    mirgen.symbol_table.emplace(name + "_ptr", lvar);

    mir::Constants index = count++;
    auto ptrtoval = exprvisitor.emplace(
        minst::Field{{name, type}, rvar, std::make_shared<mir::Value>(std::move(index))});
    auto valtostore = exprvisitor.emplace(minst::Load{{mirgen.makeNewName(), type}, ptrtoval});
    exprvisitor.emplace(minst::Store{{name, type}, lvar, valtostore});
  }
}

void StatementKnormVisitor::operator()(ast::Assign& ast) {
  auto visitor = AssignKnormVisitor(exprvisitor.mirgen, exprvisitor, ast.expr);
  std::visit(visitor, ast.lvar);
}
void StatementKnormVisitor::operator()(ast::Return& ast) {
  auto val = exprvisitor.genInst(ast.value);
  auto type = mir::getType(*val);

  this->retvalue =
      exprvisitor.emplace(minst::Return{{exprvisitor.mirgen.makeNewName(), type}, val});
}
// Instructions StatementKnormVisitor::operator()(ast::Declaration& ast){}
void StatementKnormVisitor::operator()(ast::For& /*ast*/) {
  // TODO(tomyoa)
}

void StatementKnormVisitor::operator()(ast::Fcall& ast) {
  exprvisitor.genFcallInst(ast, std::nullopt);
}
void StatementKnormVisitor::operator()(ast::Time& ast) {
  exprvisitor.genFcallInst(ast.fcall, exprvisitor.genInst(ast.when));
}

}  // namespace mimium