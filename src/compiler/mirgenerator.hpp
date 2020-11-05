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
      : statementvisitor(*this), exprvisitor(*this), typeenv(typeenv), fnctx(std::nullopt) {}
  struct ExprKnormVisitor : public VisitorBase<mir::valueptr&> {
    explicit ExprKnormVisitor(MirGenerator& parent) : mirgen(parent) {}
    mir::valueptr operator()(ast::Op& ast);
    mir::valueptr operator()(ast::Number& ast);
    mir::valueptr operator()(ast::String& ast);
    mir::valueptr operator()(ast::Symbol& ast);
    mir::valueptr operator()(ast::Self& ast);
    mir::valueptr operator()(ast::Lambda& ast);
    mir::valueptr operator()(ast::Fcall& ast);
    mir::valueptr operator()(ast::Struct& ast);
    mir::valueptr operator()(ast::StructAccess& ast);
    mir::valueptr operator()(ast::ArrayInit& ast);
    mir::valueptr operator()(ast::ArrayAccess& ast);
    mir::valueptr operator()(ast::Tuple& ast);
    mir::valueptr operator()(ast::If& ast);
    mir::valueptr operator()(ast::Block& ast);
    mir::valueptr genInst(ast::ExprPtr expr) { return std::visit(*this, *expr); }

   private:
    MirGenerator& mirgen;
  };
  struct AssignKnormVisitor {
    explicit AssignKnormVisitor(MirGenerator& parent, ast::ExprPtr expr)
        : mirgen(parent), expr(std::move(expr)){};
    void operator()(ast::DeclVar& ast);
    void operator()(ast::ArrayLvar& ast);
    void operator()(ast::TupleLvar& ast);

   private:
    MirGenerator& mirgen;
    ast::ExprPtr expr;
  };
  struct StatementKnormVisitor {
    explicit StatementKnormVisitor(MirGenerator& parent) : mirgen(parent), retvalue(std::nullopt) {}
    void operator()(ast::Assign& ast);
    void operator()(ast::Fdef& ast);
    void operator()(ast::Return& ast);
    void operator()(ast::Time& ast);
    void operator()(ast::Fcall& ast);
    void operator()(ast::For& ast);
    void operator()(ast::If& ast);
    void genInst(ast::Statement stmt) { return std::visit(*this, stmt); }

    optvalptr retvalue;

   private:
    MirGenerator& mirgen;
  };
  mir::blockptr generate(ast::Statements& topast);

 private:
  std::pair<optvalptr, mir::blockptr> generateBlock(ast::Block& block, std::string label,
                                                    optvalptr const& fnctx);

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
    mir::valueptr val = mir::addInstToBlock(std::move(inst), block_ctx);
    // if (auto* v = val.value_or(nullptr)) {
    //   // todo: redundunt data store?
    //   symbol_table.emplace(v->name, mir::Value{v->name, v->type});
    //   typeenv.emplace(v->name, v->type);
    // }
    return val;
  }
  // expect return value
  auto emplaceExpr(mir::Instructions&& inst) { return require(emplace(std::move(inst))); }
  template <typename FROM, typename TO, class LAMBDA>
  auto transformArgs(FROM&& from, TO&& to, LAMBDA&& op) -> decltype(to) {
    std::transform(from.begin(), from.end(), std::back_inserter(to), op);
    return std::forward<decltype(to)>(to);
  }

  mir::valueptr genFcallInst(ast::Fcall& fcall, optvalptr const& when = std::nullopt);
  std::pair<optvalptr, mir::blockptr> genIfBlock(ast::ExprPtr& block, std::string const& label);
  optvalptr genIfInst(ast::If& ast, bool is_expr);
  mir::valueptr genInst(ast::ExprPtr expr) { return exprvisitor.genInst(expr); }
  void genInst(ast::Statement stmt) { return statementvisitor.genInst(stmt); }
  mir::valueptr genAllocate( std::string const& name, types::Value const& type);

  static bool isPassByValue(types::Value const& type);

  StatementKnormVisitor statementvisitor;
  ExprKnormVisitor exprvisitor;
  optvalptr fnctx = nullptr;
  mir::blockptr block_ctx = nullptr;
  int indent_counter = 0;
  int64_t varcounter = 0;
  std::string makeNewName();
  TypeEnv& typeenv;
  std::unordered_map<std::string, mir::valueptr> symbol_table;
  std::unordered_map<std::string, mir::valueptr> external_symbols;

  // static bool isExternalFun(std::string const& str) {
  //   return LLVMBuiltin::ftable.find(str) != LLVMBuiltin::ftable.end();
  // }
  mir::valueptr getOrGenExternalSymbol(std::string const& name, types::Value const& type);
  mir::valueptr getInternalSymbol(std::string const& name);
  optvalptr tryGetInternalSymbol(std::string const& name);

  // // unpack optional value ptr, and throw error if it does not exist.
  static mir::valueptr require(optvalptr const& v);
};

}  // namespace mimium