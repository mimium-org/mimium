#pragma once

#include <list>
#include "type.hpp"

namespace mimium {

template <template <class> class C>
struct MakeRecursive {
  using asts = C<Box<MakeRecursive>>;
  using type = typename asts::Expr;
};

template <class EXPR>
struct Ast {
  template <template <class...> class Category, class T, int ID = 0>
  struct Aggregate {
    Category<T> v;
    using type = Category<T>;
  };

  template <class T>
  using IdentCategory = T;

  template <class T, int ID = 0>
  using Primitive = Aggregate<IdentCategory, T, ID>;

  using FloatLit = Primitive<float>;
  using IntLit = Primitive<int>;
  using BoolLit = Primitive<bool>;
  using StringLit = Primitive<std::string>;
  using Symbol = Primitive<std::string, 1>;

  template <class T>
  using List = std::list<T>;

  using TupleLit = Aggregate<List, EXPR>;
  using ArrayLit = Aggregate<List, EXPR, 1>;
  struct StructKey {
    std::string key;
    EXPR v;
  };
  using StructLit = Aggregate<List, StructKey>;

  struct Id {
    std::string v;
  };
  template <class T>
  struct LambdaCategory {
    List<Id> args;
    T body;
  };
  using Lambda = typename Aggregate<LambdaCategory, EXPR>::type;
  template <class T>
  struct LetCategory {
    std::string id;
    T expr;
    T body;
  };
  using Let = typename Aggregate<LetCategory, EXPR>::type;
  template <class T>
  struct IfCategory {
    T cond;
    T vthen;
    std::optional<T> velse;
  };
  using If = typename Aggregate<IfCategory, EXPR>::type;

  // clang-format off
template <typename... Addtionals>
using ExprBase = std::variant<FloatLit, 
                                IntLit, 
                                BoolLit, 
                                StringLit, 
                                Symbol, 
                                TupleLit, 
                                StructLit,
                                ArrayLit,
                                Lambda, 
                                If, 
                                Addtionals...>;
  // clang-format on
  template <class T>
  struct AppCategory {
    T callee;
    List<EXPR> args;
  };
  using App = typename Aggregate<AppCategory, EXPR>::type;

  // the Ast struct has to declare Expr type to use in MakeRecursive Container.
  using Expr = ExprBase<App, Let>;
};
using ExprPrim = MakeRecursive<Ast>;

ExprPrim::type e{ExprPrim::asts::FloatLit{0.0}};
auto& test = std::get<ExprPrim::asts::FloatLit>(e);

// HIGH-LEVEL AST including Syntactic Sugar
template <class EXPR>
struct Hast {
  template <template <class...> class T, class U, int ID = 0>
  using Aggregate = ExprPrim::asts::Aggregate<T, U, ID>;

  struct CurryPlaceHolder {};
  using CurryArg = std::variant<EXPR, CurryPlaceHolder>;
  using App = typename Ast<CurryArg>::App;
  struct Op {
    std::string v;
  };
  template <class T>
  struct InfixCategory {
    Op op;
    T lhs;
    T rhs;
  };

  using Infix = typename Aggregate<InfixCategory, EXPR>::type;
  template <class T>
  struct ScheduleCategory {
    App expr;
    T time;
  };
  using Schedule = typename Aggregate<ScheduleCategory, EXPR>::type;
  struct EnvVar {
    std::string id;
  };
  template <class T>
  struct AssignmentProto {
    std::string id;
    T expr;
  };
  using Assignment = AssignmentProto<EXPR>;
  template <class T>
  using LCategory = ExprPrim::asts::LambdaCategory<T>;
  using DefFnRvalue = typename Aggregate<LCategory, EXPR>::type;
  using DefFn = AssignmentProto<DefFnRvalue>;

  using Statements = std::variant<Assignment, DefFn, ExprPrim::asts::App>;

  //   using ExtFun;TODO
  using Block = Aggregate<std::list, Statements>;
  using Expr = ExprPrim::asts::ExprBase<Infix, Schedule, EnvVar, App, Block>;
};

using HExpr = MakeRecursive<Hast>;

struct TypeAlias;  // type name = hoge

}  // namespace mimium