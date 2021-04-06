#pragma once

#include <list>
#include "abstractions.hpp"
#include "type.hpp"

namespace mimium {

template <class EXPR>
struct Ast {
  template <template <class...> class Category, class T, int ID = 0>
  using Aggregate = CategoryWrapped<Category, T, ID>;

  template <template <class...> class Category, int ID = 0>
  using Wrapper = Aggregate<IdentCategory, EXPR, ID>;

  template <class T, int ID = 0>
  using Primitive = Aggregate<IdentCategory, T, ID>;

  using FloatLit = Primitive<float>;
  using IntLit = Primitive<int>;
  using BoolLit = Primitive<bool>;
  using StringLit = Primitive<std::string>;
  using Symbol = Primitive<std::string, 1>;

  using TupleLit = Wrapper<List, 0>;

  template <class T>
  struct GetterCategory {
    EXPR expr;
    T field;
  };
  template <class T>
  using Getter = typename Aggregate<GetterCategory, T>::type;

  using TupleGet = Getter<int>;

  using ArrayLit = Wrapper<List, 1>;
  using ArrayGet = Getter<float>;
  using ArraySize = Wrapper<IdentCategory, 2>;

  struct Id {
    std::string v;
  };
  struct StructKey {
    std::string key;
    EXPR v;
  };
  using StructLit = Aggregate<List, StructKey>;

  using StructGet = Getter<std::string>;

  template <class T>
  struct LambdaCategory {
    List<Id> args;
    T body;
  };
  using Lambda = typename Aggregate<LambdaCategory, EXPR>::type;

  template <template <class...> class IdContainer>
  struct LetCategory {
    template <class T>
    struct Type {
      IdContainer<Id> id;
      T expr;
      T body;
    };
  };

  template <class T>
  using LetCategorySingle = typename LetCategory<IdentCategory>::template Type<T>;
  template <class T>
  using LetCategoryTuple = typename LetCategory<List>::template Type<T>;

  using Let = typename Aggregate<LetCategorySingle, EXPR>::type;
  using LetTuple = typename Aggregate<LetCategoryTuple, EXPR>::type;

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
                                TupleGet, 
                                StructLit,
                                StructGet,
                                ArrayLit,
                                ArrayGet,
                                ArraySize,
                                Lambda, 
                                If, 
                                Addtionals...>;
  // clang-format on
  template <class T>
  struct AppCategory {
    T callee;
    List<EXPR> args;
  };
  using App = Wrapper<AppCategory>;

  // the Ast struct has to declare Expr type to use in MakeRecursive Container.
  using Expr = ExprBase<App, Let>;
};
using ExprPrim = MakeRecursive<Ast>::type;

ExprPrim::Expr e{ExprPrim::FloatLit{0.0}};
auto& test = std::get<ExprPrim::FloatLit>(e);

// HIGH-LEVEL AST including Syntactic Sugar
template <class EXPR>
struct Hast {
  template <template <class...> class T, class U, int ID = 0>
  using Aggregate = ExprPrim::Aggregate<T, U, ID>;

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
  using LCategory = ExprPrim::LambdaCategory<T>;
  using DefFnRvalue = typename Aggregate<LCategory, EXPR>::type;
  using DefFn = AssignmentProto<DefFnRvalue>;

  using Return = typename Aggregate<IdentCategory, EXPR, 1>::type;

  using Statement = std::variant<Assignment, DefFn, ExprPrim::App>;

  //   using ExtFun;TODO
  struct Block {
    Aggregate<std::list, Statement> statements;
    std::optional<Return> ret;
  };

  using Expr = ExprPrim::ExprBase<Infix, Schedule, EnvVar, App, Block>;
};

using HExpr = MakeRecursive<Hast>::type;

struct TypeAlias;  // type name = hoge

}  // namespace mimium