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
  bool operator()(types::Function& t) {
    return std::visit(*this, t.ret_type) || checkArgs(t.arg_types);
  }
  bool operator()(types::Array& t) { return std::visit(*this, t.elem_type); }
  bool operator()(types::Struct& t) { return checkArgs(t.arg_types); }
  bool operator()(types::Tuple& t) { return checkArgs(t.arg_types); };
  bool operator()(types::TypeVar& t) { return (t.index == tv.index); }
  template <typename T>
  bool operator()(T& t) {
    bool res;
    if constexpr (T::kind == types::Kind::POINTER) {
      res = std::visit(*this, t.val);
    } else if constexpr (std::is_same_v<std::decay_t<T>, types::Alias>) {
      res = std::visit(*this, t.target);
    } else {
      res = false;
    }
    return res;
  }
  template <typename T>
  bool operator()(Rec_Wrap<T>& t) {
    return (*this)(t.getraw());
  }
  bool checkArgs(std::vector<types::Value>& args) {
    bool res = false;
    for (auto&& a : args) {
      res |= std::visit(*this, a);
    }
    return res;
  }
  bool checkArgs(std::vector<types::Struct::Keytype>& args) {
    bool res = false;
    for (auto&& a : args) {
      res |= std::visit(*this, a.val);
    }
    return res;
  }
};

struct TypeInferer {
  struct ExprTypeVisitor : public VisitorBase<types::Value> {
    explicit ExprTypeVisitor(TypeInferer& parent) : inferer(parent) {}
    types::Value operator()(ast::Op& ast);
    types::Value operator()(ast::Number& ast);
    types::Value operator()(ast::String& ast);
    types::Value operator()(ast::Rvar& ast);
    types::Value operator()(ast::Self& ast);
    types::Value operator()(ast::Lambda& ast);
    types::Value operator()(ast::Fcall& ast);
    types::Value operator()(ast::Time& ast);
    types::Value operator()(ast::Struct& ast);
    types::Value operator()(ast::StructAccess& ast);
    types::Value operator()(ast::ArrayInit& ast);
    types::Value operator()(ast::ArrayAccess& ast);
    types::Value operator()(ast::Tuple& ast);

   private:
    TypeInferer& inferer;
  };
  // visitor for ast::Statements. its return value will be the return/expr of
  // last line in statements(used for inference of function return type).
  struct StatementTypeVisitor : public VisitorBase<types::Value> {
    explicit StatementTypeVisitor(TypeInferer& parent) : inferer(parent) {}
    types::Value operator()(ast::Assign& ast);
    types::Value operator()(ast::Return& ast);
    // types::Value operator()(ast::Declaration& ast);
    types::Value operator()(ast::For& ast);
    types::Value operator()(ast::If& ast);
    types::Value operator()(ast::ExprPtr& ast);

   private:
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
    types::Value unify(types::rAlias a1, types::rAlias a2);
    template <typename T>
    types::Value unify(T a1, T a2) {
      return a1;
    }

    template <typename T, typename std::enable_if<
                              std::is_same_v<std::decay_t<T>, types::TypeVar>>::type=nullptr>
    types::Value unify(types::rAlias a1, T a2) {
      return std::visit(*this, a1.getraw().target, types::Value(a2));
    }
    template <typename T, typename std::enable_if<
                              std::is_same_v<std::decay_t<T>, types::TypeVar>>::type=nullptr>
    types::Value unify(T a1, types::rAlias a2) {
      return std::visit(*this, types::Value(a1), a2.getraw().target);
    }
    types::Value unify(types::rFunction f1, types::rFunction f2);
    types::Value unify(types::rArray a1, types::rArray a2);
    types::Value unify(types::rStruct s1, types::rStruct s2);
    types::Value unify(types::rTuple f1, types::rTuple f2);

    // note(tomoya): t2 in debugger may be desplayed as "error: no value"
    // despite they are active(because of expansion of parameter pack in
    // std::visit)
    template <typename T1, typename T2>
    types::Value operator()(T1&& t1, T2&& t2) {
      constexpr bool issame =
          std::is_same_v<std::decay_t<T1>, std::decay_t<T2>>;
      constexpr bool istv_l = std::is_same_v<std::decay_t<T1>, types::rTypeVar>;
      constexpr bool istv_r = std::is_same_v<std::decay_t<T2>, types::rTypeVar>;
      if constexpr (issame || istv_l || istv_r) {
        return unify(t1, t2);
      } else {
        throw std::runtime_error("type mismatch");
        return t1;  // for primitives
      }
    }
    std::vector<types::Value> unifyArgs(std::vector<types::Value>& v1,
                                        std::vector<types::Value>& v2);
  };
  struct SubstituteVisitor {
    explicit SubstituteVisitor(TypeInferer& parent) : inferer(parent) {}
    types::Value operator()(types::TypeVar& t) {
      auto& target = inferer.typeenv.findTypeVar(t.index);
      if (std::visit(OccurChecker{t}, target)) {
        Logger::debug_log( "type loop detected. decuced into float type.",Logger::WARNING);
        return types::Float();
      }
      auto contained = std::visit(*this,target);
      if (std::holds_alternative<types::None>(contained) ||
          std::holds_alternative<types::rTypeVar>(contained)) {
        throw std::runtime_error(
            "failed to replace typevar. decuced into float type.");
        return types::Float();
      }
      return std::visit(*this, target);
    }
    types::Value operator()(types::Function& f) {
      return types::Function(std::visit(*this, f.ret_type),
                             replaceArgs(f.arg_types));
    }
    types::Value operator()(types::Alias& a) {
      return types::Alias(a.name, std::visit(*this, a.target));
    }
    // TODO(tomoya): other aggregate types...
    template <typename T>
    types::Value operator()(T& t) {
      if constexpr (T::kind == types::Kind::POINTER) {
        return T(std::visit(*this, t.val));
      }
      return t;
    }
    template <typename T>
    types::Value operator()(Rec_Wrap<T>& t) {
      return (*this)(t.getraw());
    }
    std::vector<types::Value> replaceArgs(std::vector<types::Value>& arg) {
      std::vector<types::Value> res;
      for (auto&& a : arg) {
        res.emplace_back(std::visit(*this, a));
      }
      return res;
    }
    TypeInferer& inferer;
  };

  friend struct ExprTypeVisitor;
  friend struct StatementTypeVisitor;
  friend struct TypeUnifyVisitor;
  TypeInferer()
      : exprvisitor(*this),
        statementvisitor(*this),
        unifyvisitor(*this),
        substitutevisitor(*this),
        typeenv() {
    for (const auto& [key, val] : LLVMBuiltin::ftable) {
      typeenv.emplace(key, val.mmmtype);
    }
    typeenv.emplace("mimium_getnow", types::Function(types::Float(), {}));
  }
  // entry point.
  TypeEnv& infer(ast::Statements& topast);
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
  types::Value addLvar(ast::Lvar& lvar);
  types::Value unify(types::Value lhs, types::Value rhs) {
    return std::visit(unifyvisitor, lhs, rhs);
  }

  void substituteTypeVars();
};

}  // namespace mimium