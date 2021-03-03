/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/ast.hpp"
#include "basic/helper_functions.hpp"
#include "basic/type.hpp"
#include "compiler/ffi.hpp"
// type inference ... assumed to be visited after finished alpha-conversion(each
// variable has unique name regardless its scope)

namespace mimium {
struct OccurChecker {
  types::TypeVar& tv;
  explicit OccurChecker(types::TypeVar& target) : tv(target) {}
  bool operator()(const types::Function& t) const {
    return std::visit(*this, t.ret_type) || checkArgs(t.arg_types);
  }
  bool operator()(const types::Array& t) const { return std::visit(*this, t.elem_type); }
  bool operator()(const types::Struct& t) const { return checkArgs(t.arg_types); }
  bool operator()(const types::Tuple& t) const { return checkArgs(t.arg_types); };
  bool operator()(const types::TypeVar& t) const { return (t.index == tv.index); }
  template <typename T>
  bool operator()(const Box<T>& t) const {
    return (*this)(static_cast<T const&>(t));
  }
  template <typename T>
  bool operator()(const T& t) const {
    if constexpr (types::is_pointer_t<T>) {
      return std::visit(*this, t.val);
    } else if constexpr (std::is_same_v<std::decay_t<T>, types::Alias>) {
      return std::visit(*this, t.target);
    }
    return false;
  }
  [[nodiscard]] bool checkArgs(std::vector<types::Value> const& args) const {
    return std::accumulate(
        args.begin(), args.end(), false,
        [&](bool b, types::Value const& val) -> bool { return b || std::visit(*this, val); });
  }
  [[nodiscard]] bool checkArgs(std::vector<types::Struct::Keytype> const& args) const {
    return std::accumulate(args.begin(), args.end(), false,
                           [&](bool b, types::Struct::Keytype const& a) -> bool {
                             return b || std::visit(*this, a.val);
                           });
  }
};

struct TypeInferer {
  struct ExprTypeVisitor : public VisitorBase<types::Value> {
    explicit ExprTypeVisitor(TypeInferer& parent) : inferer(parent) {}
    types::Value operator()(ast::Op& ast);
    types::Value operator()(ast::Number& ast);
    types::Value operator()(ast::String& ast);
    types::Value operator()(ast::Symbol& ast);
    types::Value operator()(ast::Self& ast);
    types::Value operator()(ast::Lambda& ast);
    types::Value operator()(ast::Fcall& ast);
    types::Value operator()(ast::Struct& ast);
    types::Value operator()(ast::StructAccess& ast);
    types::Value operator()(ast::ArrayInit& ast);
    types::Value operator()(ast::ArrayAccess& ast);
    types::Value operator()(ast::Tuple& ast);

    types::Value operator()(ast::If& ast);
    types::Value operator()(ast::Block& ast);
    types::Value infer(ast::ExprPtr expr) { return std::visit(*this, *expr); }

   private:
    TypeInferer& inferer;
  };
  // visitor for ast::Statements. its return value will be the return/expr of
  // last line in statements(used for inference of function return type).
  struct StatementTypeVisitor : public VisitorBase<types::Value> {
    explicit StatementTypeVisitor(TypeInferer& parent) : inferer(parent) {}
    types::Value operator()(ast::Fdef& ast);
    types::Value operator()(ast::Assign& ast);
    types::Value operator()(ast::TypeAssign& ast);
    types::Value operator()(ast::Return& ast);
    types::Value operator()(ast::Time& ast);

    // types::Value operator()(ast::Declaration& ast);
    types::Value operator()(ast::For& ast);
    types::Value operator()(ast::If& ast);
    types::Value operator()(ast::Fcall& ast);
    types::Value infer(ast::Statement stmt) { return std::visit(*this, stmt); }

   private:
    TypeInferer& inferer;
  };
  struct LvarTypeVisitor {
    explicit LvarTypeVisitor(TypeInferer& parent, ast::ExprPtr rvar)
        : rvar(std::move(rvar)), inferer(parent) {}

    void operator()(ast::DeclVar& ast);
    void operator()(ast::TupleLvar& ast);
    void operator()(ast::ArrayLvar& ast);

   private:
    ast::ExprPtr rvar;
    TypeInferer& inferer;
  };

  struct TypeUnifyVisitor {
    explicit TypeUnifyVisitor(TypeInferer& parent) : inferer(parent) {}
    TypeInferer& inferer;

    // // typevar unifying
    template <typename T>
    types::Value unify(T t1, types::rTypeVar t2) {
      types::Value res = t1;
      auto& t2_real = inferer.typeenv.findTypeVar(t2.getraw().index);
      if (!std::holds_alternative<types::rTypeVar>(t2_real)) {
        res = inferer.unify(t2_real, types::Value(t1));
      } else {
        if (OccurChecker{rv::get<types::TypeVar>(t2_real)}(t1)) {
          throw std::runtime_error("type loop detected");
        }
        inferer.typeenv.tv_container[rv::get<types::TypeVar>(t2_real).index] = t1;
      }
      return res;
    }
    template <typename T>
    types::Value unify(types::rTypeVar t1, T t2) {
      return unify(t2, t1);  //
    }
    types::Value unify(types::rTypeVar t1, types::rTypeVar t2);
    types::Value unify(types::rPointer p1, types::rPointer p2);
    types::Value unify(types::rRef p1, types::rRef p2);
    types::Value unify(types::Alias a1, types::Alias a2);
    template <typename T>
    types::Value unify(T a1, T /*a2*/) {
      return a1;
    }
    template <typename T>
    using TypeVarAliasTrait =
        typename std::enable_if<std::is_same_v<std::decay_t<T>, types::TypeVar>>::type;

    template <typename T, TypeVarAliasTrait<T>>
    types::Value unify(types::Alias a1, T a2) {
      return std::visit(*this, a1.target, types::Value(a2));
    }
    template <typename T, TypeVarAliasTrait<T>>
    types::Value unify(T a1, types::rAlias a2) {
      return unify(a2, a1);
    }
    types::Value unify(types::rFunction f1, types::rFunction f2);
    types::Value unify(types::rArray a1, types::rArray a2);
    types::Value unify(types::rStruct s1, types::rStruct s2);
    types::Value unify(types::rTuple t1, types::rTuple t2);

    // note(tomoya): t2 in debugger may be desplayed as "error: no value"
    // despite they are active(because of expansion of parameter pack in
    // std::visit)
    template <typename T1, typename T2>
    types::Value operator()(T1&& t1, T2&& t2) {
      constexpr bool issame = std::is_same_v<std::decay_t<T1>, std::decay_t<T2>>;
      constexpr bool istv_l = std::is_same_v<std::decay_t<T1>, types::rTypeVar>;
      constexpr bool istv_r = std::is_same_v<std::decay_t<T2>, types::rTypeVar>;
      if constexpr (issame || istv_l || istv_r) { return unify(t1, t2); }
      throw std::runtime_error("type mismatch");
    }
    std::vector<types::Value> unifyArgs(std::vector<types::Value>& v1,
                                        std::vector<types::Value>& v2);
  };
  struct SubstituteVisitor {
    explicit SubstituteVisitor(TypeInferer& parent) : inferer(parent) {}
    types::Value operator()(types::TypeVar& t) {
      auto& target = inferer.typeenv.findTypeVar(t.index);
      if (std::visit(OccurChecker{t}, target)) {
        Logger::debug_log("type loop detected. decuced into float type.", Logger::WARNING);
        return types::Float{};
      }
      types::Value contained = std::visit(*this, target);
      if (std::holds_alternative<types::None>(contained) ||
          std::holds_alternative<types::rTypeVar>(contained)) {
        throw std::runtime_error("failed to replace typevar. decuced into float type.");
        return types::Float{};
      }
      return std::visit(*this, target);
    }
    types::Value operator()(types::Float& t) { return t; }
    types::Value operator()(types::String& t) { return t; }
    types::Value operator()(types::Void& t) { return t; }
    types::Value operator()(types::None& t) { return t; }
    types::Value operator()(types::Closure& t) {
      return t;
    }  // closure will not be shown at this stage

    types::Value operator()(types::Pointer& t) { return types::Pointer{std::visit(*this, t.val)}; }
    types::Value operator()(types::Ref& t) { return types::Pointer{std::visit(*this, t.val)}; }
    types::Value operator()(types::Function& f) {
      return types::Function{std::visit(*this, f.ret_type), replaceArgs(f.arg_types)};
    }
    types::Value operator()(types::Array& a) {
      return types::Array{std::visit(*this, a.elem_type), a.size};
    }
    types::Value operator()(types::Tuple& a) {
      std::vector<types::Value> res;
      std::transform(a.arg_types.begin(), a.arg_types.end(), std::back_inserter(res),
                     [&](types::Value& v) { return std::visit(*this, v); });
      return types::Tuple{std::move(res)};
    }
    types::Value operator()(types::Struct& a) {
      std::vector<types::Struct::Keytype> res;
      std::transform(a.arg_types.begin(), a.arg_types.end(), std::back_inserter(res),
                     [&](types::Struct::Keytype& v) {
                       return types::Struct::Keytype{v.field, std::visit(*this, v.val)};
                     });
      return types::Struct{std::move(res)};
    }

    types::Value operator()(types::Alias& a) {
      return types::Alias{a.name, std::visit(*this, a.target)};
    }

    std::vector<types::Value> replaceArgs(std::vector<types::Value>& arg) {
      std::vector<types::Value> res;
      res.reserve(arg.size());
      for (auto&& a : arg) { res.emplace_back(std::visit(*this, a)); }
      return res;
    }
    template <typename T>
    types::Value operator()(Box<T>& t) {
      return (*this)(static_cast<T&>(t));
    }
    TypeInferer& inferer;
  };

  friend struct ExprTypeVisitor;
  friend struct StatementTypeVisitor;
  friend struct TypeUnifyVisitor;
  TypeInferer()
      : typeenv(),
        exprvisitor(*this),
        statementvisitor(*this),
        unifyvisitor(*this),
        substitutevisitor(*this) {
    for (const auto& [key, val] : LLVMBuiltin::ftable) { typeenv.emplace(key, val.mmmtype); }
    typeenv.emplace("mimium_getnow", types::Function{types::Float{}, {}});
  }
  // entry point.
  TypeEnv& infer(ast::Statements& topast);
  types::Value inferExpr(ast::ExprPtr expr) { return exprvisitor.infer(expr); }
  types::Value inferStatement(ast::Statement expr) { return statementvisitor.infer(expr); }
  types::Value inferStatements(ast::Statements& statements);
  TypeEnv& getTypeEnv() { return typeenv; }

 private:
  TypeEnv typeenv;
  std::unordered_map<int, types::Value> typevarmap;
  std::stack<types::Value> selftype_stack;
  ExprTypeVisitor exprvisitor;
  StatementTypeVisitor statementvisitor;
  TypeUnifyVisitor unifyvisitor;
  SubstituteVisitor substitutevisitor;
  types::Value addDeclVar(ast::DeclVar& lvar);
  types::Value inferFcall(ast::Fcall& fcall);
  types::Value inferIf(ast::If& ast);

  types::Value unify(types::Value lhs, types::Value rhs) {
    return std::visit(unifyvisitor, lhs, rhs);
  }

  void substituteTypeVars();
};

}  // namespace mimium