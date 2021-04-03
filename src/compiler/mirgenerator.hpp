/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <utility>

#include "basic/mir.hpp"
#include "compiler/ffi.hpp"
#include "compiler/type_infer_visitor.hpp"

namespace mimium {

using optvalptr = std::optional<mir::valueptr>;
namespace mir {

inline types::Value lowerType(types::Value const& t) {
  return std::visit(
      overloaded_rec{
          [](auto const& i) {
            assert(types::is_primitive_type<decltype(i)>);
            return types::Value{i};
          },
          // [](types::TypeVar i) {
          //   assert(false);
          //   return types::Value{i};
          // },
          // [](types::Ref i) {
          //   assert(false);
          //   return types::Value{i};
          // },
          [](types::Pointer const& i) {
            // assert(false);
            return types::Value{i};
          },
          // [](types::Closure i) {
          //   assert(false);
          //   return types::Value{i};
          // },

          [](types::Function const& i) {
            types::Function res;
            if (types::isAggregate(i.ret_type)) {
              res.ret_type = types::Void{};
              res.arg_types.emplace_back(lowerType(i.ret_type));
            } else {
              res.ret_type = i.ret_type;
            }
            for (const auto& a : i.arg_types) { res.arg_types.emplace_back(a); }
            return types::Value{res};
          },
          [](types::Array const& i) {
            return types::Value{types::Pointer{types::Array{lowerType(i.elem_type), i.size}}};
          },
          [](types::Struct const& i) {
            return types::Value{types::Pointer{types::Struct{fmap(i.arg_types, [](auto const& a) {
              return types::Struct::Keytype{a.field, lowerType(a.val)};
            })}}};
          },
          [](types::Tuple const& i) {
            return types::Value{types::Pointer{
                types::Tuple{fmap(i.arg_types, [](auto const& i) { return lowerType(i); })}}};
          },
          [](types::Alias const& i) {
            return types::Value{types::Alias{i.name, lowerType(i.target)}};
          },
      },
      t);
}

}  // namespace mir

class MirGenerator {
 public:
  explicit MirGenerator(TypeEnv& typeenv) : typeenv(typeenv) {}
  struct ExprKnormVisitor : public VisitorBase<mir::valueptr&> {
    explicit ExprKnormVisitor(MirGenerator& parent, mir::blockptr block,
                              const std::optional<mir::valueptr>& fnctx)
        : mirgen(parent), fnctx(fnctx), block(std::move(block)) {}
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
    mir::valueptr genInst(ast::ExprPtr expr,
                          std::optional<std::string> const& lvar = std::nullopt) {
      lvar_holder = lvar;
      auto res = std::visit(*this, *expr);
      lvar_holder = std::nullopt;
      return res;
    }

    MirGenerator& mirgen;
    mir::valueptr emplace(mir::Instructions&& inst);
    mir::valueptr genAllocate(std::string const& name, types::Value const& type);
    mir::valueptr genFcallInst(ast::Fcall& fcall, optvalptr const& when);

    const std::optional<mir::valueptr>& fnctx;

   private:
    mir::valueptr genExprArray(std::deque<ast::ExprPtr> const& args);
    std::pair<optvalptr, mir::blockptr> genIfBlock(ast::ExprPtr& block, std::string const& label);

    mir::blockptr block;
    std::optional<std::string> lvar_holder;
  };
  struct AssignKnormVisitor {
    explicit AssignKnormVisitor(MirGenerator& parent, ExprKnormVisitor& exprvisitor,
                                ast::ExprPtr expr)
        : mirgen(parent), exprvisitor(exprvisitor), expr(std::move(expr)){};
    void operator()(ast::DeclVar& ast);
    void operator()(ast::ArrayLvar& ast);
    void operator()(ast::TupleLvar& ast);
    void operator()(ast::StructLvar& ast);

   private:
    MirGenerator& mirgen;
    ExprKnormVisitor& exprvisitor;

    ast::ExprPtr expr;
  };
  struct StatementKnormVisitor {
    explicit StatementKnormVisitor(ExprKnormVisitor& evisitor)
        : retvalue(std::nullopt), exprvisitor(evisitor), mirgen(evisitor.mirgen) {}

    void operator()(ast::Assign& ast);
    void operator()(ast::TypeAssign& ast);
    void operator()(ast::Fdef& ast);
    void operator()(ast::Return& ast);
    void operator()(ast::Time& ast);
    void operator()(ast::Fcall& ast);
    void operator()(ast::For& ast);
    void operator()(ast::If& ast);
    void genInst(ast::Statement& stmt) { return std::visit(*this, stmt); }

    optvalptr retvalue;

   private:
    ExprKnormVisitor& exprvisitor;
    MirGenerator& mirgen;
  };
  mir::blockptr generate(ast::Statements& topast);

 private:
  std::pair<optvalptr, mir::blockptr> generateBlock(ast::Block& block, std::string label,
                                                    optvalptr const& fnctx);

  optvalptr genIfInst(ast::If& ast);

  static bool isPassByValue(types::Value const& type);

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
  mir::valueptr getFunctionSymbol(std::string const& name, types::Value const& type);
  // // unpack optional value ptr, and throw error if it does not exist.
  static mir::valueptr require(optvalptr const& v);

  std::function<std::shared_ptr<mir::Argument>(ast::DeclVar)> make_arguments =
      [&](ast::DeclVar lvar) {
        auto& name = lvar.value.value;
        auto type = typeenv.find(name);
        if (!isPassByValue(type)) { type = types::makePointer(type); }
        auto res = std::make_shared<mir::Argument>(mir::Argument{name, type});
        symbol_table.emplace(name, std::make_shared<mir::Value>(res));
        return res;
      };
};

}  // namespace mimium