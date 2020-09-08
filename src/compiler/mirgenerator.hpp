/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/mir.hpp"
#include "compiler/ffi.hpp"
#include "compiler/type_infer_visitor.hpp"

namespace mimium {

using lvarid = std::pair<std::string, types::Value>;
class MirGenerator {
 public:
  explicit MirGenerator(TypeEnv& typeenv)
      : statementvisitor(*this),
        exprvisitor(*this),
        typeenv(typeenv),
        ctx(nullptr) {}
  struct ExprKnormVisitor : public VisitorBase<lvarid&> {
    explicit ExprKnormVisitor(MirGenerator& parent) : mirgen(parent) {}
    lvarid operator()(ast::Op& ast);
    lvarid operator()(ast::Number& ast);
    lvarid operator()(ast::String& ast);
    lvarid operator()(ast::Rvar& ast);
    lvarid operator()(ast::Self& ast);
    lvarid operator()(ast::Lambda& ast);
    lvarid operator()(ast::Fcall& ast,
                      std::optional<std::string> when = std::nullopt);
    lvarid operator()(ast::Struct& ast);
    lvarid operator()(ast::StructAccess& ast);
    lvarid operator()(ast::ArrayInit& ast);
    lvarid operator()(ast::ArrayAccess& ast);
    lvarid operator()(ast::Tuple& ast);
    lvarid operator()(ast::If& ast);
    lvarid operator()(ast::Block& ast);
    lvarid genInst(ast::ExprPtr expr) { return std::visit(*this, *expr); }

   private:
    MirGenerator& mirgen;
  };
  struct StatementKnormVisitor : public VisitorBase<lvarid&> {
    explicit StatementKnormVisitor(MirGenerator& parent) : mirgen(parent) {}
    lvarid operator()(ast::Assign& ast);
    lvarid operator()(ast::Fdef& ast);
    lvarid operator()(ast::Return& ast);
    lvarid operator()(ast::Time& ast);
    lvarid operator()(ast::Fcall& ast);
    lvarid operator()(ast::For& ast);
    lvarid operator()(ast::If& ast);
    // mir::Instructions operator()(ast::Declaration& ast);
    lvarid genInst(ast::Statement stmt) { return std::visit(*this, stmt); }

   private:
    MirGenerator& mirgen;
  };
  mir::blockptr generate(ast::Statements& topast);
  std::pair<lvarid, mir::blockptr> generateBlock(ast::Block& block,
                                                             std::string label);
  bool isOverWrite(std::string const& name) {
    return std::find(lvarlist.begin(), lvarlist.end(), name) != lvarlist.end();
  }
  lvarid emplace(mir::Instructions&& inst, types::Value type = types::Float()) {
    auto& newname =
        std::visit([](auto&& i) -> std::string& { return i.lv_name; },
                   mir::addInstToBlock(std::move(inst),ctx));
    typeenv.emplace(newname, type);
    return std::pair(newname, type);
  }
  template <typename FROM, typename TO, class LAMBDA>
  auto transformArgs(FROM&& from, TO&& to, LAMBDA&& op) -> decltype(to) {
    std::transform(from.begin(), from.end(), std::back_inserter(to), op);
    return std::forward<decltype(to)>(to);
  }
  static bool isExternalFun(std::string const& str) {
    return LLVMBuiltin::ftable.find(str) != LLVMBuiltin::ftable.end();
  }
  lvarid genFcallInst(ast::Fcall& fcall,
                      std::optional<std::string> when = std::nullopt);
  std::pair<lvarid, mir::blockptr> genIfBlock(
      ast::ExprPtr& block, std::string const& label);
  lvarid genIfInst(ast::If& ast);
  lvarid genInst(ast::ExprPtr expr) { return exprvisitor.genInst(expr); }
  lvarid genInst(ast::Statement stmt) { return statementvisitor.genInst(stmt); }

 private:
  StatementKnormVisitor statementvisitor;
  ExprKnormVisitor exprvisitor;
  TypeEnv& typeenv;
  std::vector<std::string> lvarlist;
  std::optional<std::string> lvar_holder;
  mir::blockptr ctx = nullptr;
  std::stack<types::Value> selftype_stack;
  int64_t varcounter = 0;
  std::string makeNewName();
};

}  // namespace mimium