/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/mir.hpp"
#include "compiler/ffi.hpp"
#include "compiler/type_infer_visitor.hpp"

namespace mimium {

using optvalptr = std::optional<mir::valueptr>;
class MirGenerator {
 public:
  explicit MirGenerator(TypeEnv& typeenv)
      : statementvisitor(*this), exprvisitor(*this), typeenv(typeenv), ctx(nullptr) {}
  struct ExprKnormVisitor : public VisitorBase<mir::valueptr&> {
    explicit ExprKnormVisitor(MirGenerator& parent) : mirgen(parent) {}
    optvalptr operator()(ast::Op& ast);
    optvalptr operator()(ast::Number& ast);
    optvalptr operator()(ast::String& ast);
    optvalptr operator()(ast::Symbol& ast);
    optvalptr operator()(ast::Self& ast);
    optvalptr operator()(ast::Lambda& ast);
    optvalptr operator()(ast::Fcall& ast);
    optvalptr operator()(ast::Struct& ast);
    optvalptr operator()(ast::StructAccess& ast);
    optvalptr operator()(ast::ArrayInit& ast);
    optvalptr operator()(ast::ArrayAccess& ast);
    optvalptr operator()(ast::Tuple& ast);
    optvalptr operator()(ast::If& ast);
    optvalptr operator()(ast::Block& ast);
    optvalptr genInst(ast::ExprPtr expr) { return std::visit(*this, *expr); }

   private:
    MirGenerator& mirgen;
  };
  struct AssignKnormVisitor {
    explicit AssignKnormVisitor(MirGenerator& parent, ast::ExprPtr expr)
        : mirgen(parent), expr(std::move(expr)){};
    optvalptr operator()(ast::DeclVar& ast);
    optvalptr operator()(ast::ArrayLvar& ast);
    optvalptr operator()(ast::TupleLvar& ast);

   private:
    MirGenerator& mirgen;
    ast::ExprPtr expr;
  };
  struct StatementKnormVisitor {
    explicit StatementKnormVisitor(MirGenerator& parent) : mirgen(parent) {}
    optvalptr operator()(ast::Assign& ast);
    optvalptr operator()(ast::Fdef& ast);
    optvalptr operator()(ast::Return& ast);
    optvalptr operator()(ast::Time& ast);
    optvalptr operator()(ast::Fcall& ast);
    optvalptr operator()(ast::For& ast);
    optvalptr operator()(ast::If& ast);
    // mir::Instructions operator()(ast::Declaration& ast);
    optvalptr genInst(ast::Statement stmt) { return std::visit(*this, stmt); }

   private:
    MirGenerator& mirgen;
  };
  mir::blockptr generate(ast::Statements& topast);
  std::pair<optvalptr, mir::blockptr> generateBlock(ast::Block& block, std::string label);

  // bool isOverWrite(ast::Symbol const& symbol) {
  //   auto iter = symbol_table.find(symbol.value);
  //   return std::find(lvarlist.begin(), lvarlist.end(), symbol.value) != lvarlist.end();
  // }

  // bool isOverWrite(ast::Lvar& lvar) {
  //   if (auto* declvar = std::get_if<ast::DeclVar>(&lvar)) { return isOverWrite(declvar->value); }
  //   // other cases: array tuple
  //   return !std::holds_alternative<ast::TupleLvar>(lvar);
  //   // todo(tomoya):prevent redefinition of variable in tuple structural binding
  // }
  auto emplace(mir::Instructions&& inst) {
    std::optional<mir::valueptr> val = mir::addInstToBlock(std::move(inst), ctx);
    if (auto* v = val.value_or(nullptr)) {
      // todo: redundunt data store?
      symbol_table.emplace(v->name, mir::Value{v->name, v->type});
      typeenv.emplace(v->name, v->type);
    }
    return val;
  }
  template <typename FROM, typename TO, class LAMBDA>
  auto transformArgs(FROM&& from, TO&& to, LAMBDA&& op) -> decltype(to) {
    std::transform(from.begin(), from.end(), std::back_inserter(to), op);
    return std::forward<decltype(to)>(to);
  }
  static bool isExternalFun(std::string const& str) {
    return LLVMBuiltin::ftable.find(str) != LLVMBuiltin::ftable.end();
  }
  optvalptr genFcallInst(ast::Fcall& fcall, optvalptr const& when = std::nullopt);
  std::pair<optvalptr, mir::blockptr> genIfBlock(ast::ExprPtr& block, std::string const& label);
  optvalptr genIfInst(ast::If& ast, bool need_alloca);
  optvalptr genInst(ast::ExprPtr expr) { return exprvisitor.genInst(expr); }
  optvalptr genInst(ast::Statement stmt) { return statementvisitor.genInst(stmt); }

 private:
  StatementKnormVisitor statementvisitor;
  ExprKnormVisitor exprvisitor;
  mir::blockptr ctx = nullptr;
  std::stack<types::Value> selftype_stack;
  int64_t varcounter = 0;
  std::string makeNewName();
  TypeEnv& typeenv;
  std::unordered_map<std::string, mir::Value> symbol_table;
  std::unordered_map<std::string, mir::Value> external_symbols;
  mir::valueptr getOrGenExternalSymbol(std::string const& name, types::Value const& type);
  mir::valueptr getInternalSymbol(std::string const& name);
  optvalptr tryGetInternalSymbol(std::string const& name);

  // unpack optional value ptr, and throw error if it does not exist.
  static mir::valueptr require(optvalptr const& v);
};

}  // namespace mimium