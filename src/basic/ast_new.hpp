#pragma once

#include <list>
#include "abstractions.hpp"
#include "sexpr.hpp"
#include "type_new.hpp"

namespace mimium {

template <class EXPR>
struct ExprCommon {
  template <template <class...> class Category, class T, int ID = 0>
  using Aggregate = CategoryWrapped<Category, T, ID>;

  template <template <class...> class Category, int ID = 0>
  using Wrapper = Aggregate<Category, EXPR, ID>;

  template <class T, int ID = 0>
  using Primitive = Aggregate<IdentCategory, T, ID>;

  template <class FieldT>
  struct Getter {
    EXPR expr;
    FieldT field;
  };

  template <class FieldT>
  auto toSExpr(Getter<FieldT> const& v) {
    return cons(makeSExpr({"get"}), cons(toSExpr(v.expr), toSExpr(v.field)));
  }

  struct Id {
    std::string v;
    std::optional<ITypeOrAlias> type;
  };
  struct StructKey {
    std::string key;
    EXPR v;
  };

  template <class T, class ID>
  struct LambdaCategory {
    List<ID> args;
    T body;
  };
  template <template <class...> class IdContainer>
  struct LetCategory {
    IdContainer<Id> id;
    EXPR expr;
    EXPR body;
  };

  template <class Cond, class T>
  struct IfCategory {
    Cond cond;
    T vthen;
    std::optional<T> velse;
  };

  using FloatLit = Primitive<float>;
  using IntLit = Primitive<int>;
  using BoolLit = Primitive<bool>;
  using StringLit = Primitive<std::string>;
  using Symbol = Primitive<std::string, 1>;

  using TupleLit = Wrapper<List, 0>;
  using TupleGet = Getter<int>;

  using ArrayLit = Wrapper<List, 1>;
  using ArrayGet = Getter<float>;
  using ArraySize = Wrapper<IdentCategory, 2>;

  using StructLit = Aggregate<List, StructKey>;
  using StructGet = Getter<std::string>;

  using Lambda = LambdaCategory<EXPR, Id>;

  using Let = LetCategory<IdentCategory>;
  using LetTuple = LetCategory<List>;

  using If = IfCategory<EXPR, EXPR>;

  template <class T>
  struct AppCategory {
    EXPR callee;
    List<T> args;
  };
  using App = Wrapper<AppCategory>;
  static SExpr toSExpr(EXPR const& v) { return std::visit(visitor, v.getraw().v); }
  constexpr static auto folder = [](auto&& a, auto&& b) { return cons(a, b); };
  inline const static auto mapper = [](EXPR const& e) -> SExpr { return toSExpr(e); };
  constexpr static auto listmatcher = [](std::string const& name, auto&& a) {
    return cons(makeSExpr(name), foldl(fmap(a.v, mapper), folder));
  };
  constexpr static auto gettermatcher = [](std::string const& name, auto&& a) {
    return cons(makeSExpr(name), cons(toSExpr(a.expr), makeSExpr(std::to_string(a.field))));
  };
  inline static auto visitor = overloaded_rec{
      [](FloatLit const& a) {
        return makeSExpr({"float", std::to_string(a.v)});
      },
      [](IntLit const& a) {
        return makeSExpr({"int", std::to_string(a.v)});
      },
      [](BoolLit const& a) {
        return makeSExpr({"bool", (a.v ? std::string{"true"} : "false")});
      },
      [](StringLit const& a) {
        return makeSExpr({"string", a.v});
      },
      [](Symbol const& a) {
        return makeSExpr({"symbol", a.v});
      },
      [](TupleLit const& a) { return listmatcher("tuple", a); },
      [](TupleGet const& a) { return gettermatcher("tupleget", a); },
      [](ArrayLit const& a) { return listmatcher("array", a); },
      [](ArrayGet const& a) { return gettermatcher("arrayget", a); },
      [](ArraySize const& a) { return makeSExpr("arraysize"); },
      [](StructLit const& a) { return makeSExpr("struct"); },
      [](StructGet const& a) { return makeSExpr(""); },
      [](Lambda const& a) {
        return cons(makeSExpr("lambda"),
                    foldl(fmap(a.args, [](auto&& id) { return makeSExpr(id.v); }), folder));
      },
      [](Let const& a) { return makeSExpr(""); },
      [](LetTuple const& a) { return makeSExpr(""); },
      [](App const& a) { return makeSExpr(""); },
      [](If const& a) { return makeSExpr(""); },
      [](auto const& a) { return makeSExpr(""); }};
  static std::string toString(EXPR const& v) { return toString(toSExpr(v)); }
};
// small set of primitives before generating mir
struct LAst {
  struct expr;
  using Expr = ExprCommon<Box<expr>>;
  using FloatLit = Expr::FloatLit;
  using IntLit = Expr::IntLit;
  using BoolLit = Expr::BoolLit;
  using StringLit = Expr::StringLit;
  using Symbol = Expr::Symbol;
  using TupleLit = Expr::TupleLit;
  using TupleGet = Expr::TupleGet;
  using ArrayLit = Expr::ArrayLit;
  using ArrayGet = Expr::ArrayGet;
  using ArraySize = Expr::ArraySize;
  using Lambda = Expr::Lambda;
  using Let = Expr::Let;
  using LetTuple = Expr::LetTuple;
  using App = Expr::App;

  using If = Expr::If;
  struct expr {
    using type = std::variant<FloatLit, IntLit, BoolLit, StringLit, Symbol, TupleLit, TupleGet,
                              ArrayLit, ArrayGet, ArraySize, Lambda, Let, LetTuple, App, If>;
    type v;
    operator type&() { return v; };        // NOLINT
    operator const type&() { return v; };  // NOLINT
  };
};

// HIGH-LEVEL AST including Syntactic Sugar
template <class EXPR>
struct HastCommon {
  template <template <class...> class T, class U, int ID = 0>
  using Aggregate = typename ExprCommon<EXPR>::template Aggregate<T, U, ID>;

  struct CurryPlaceHolder {};
  using CurryArg = std::variant<EXPR, CurryPlaceHolder>;

  using App = typename ExprCommon<EXPR>::template AppCategory<CurryArg>;

  struct Op {
    std::string v;
  };
  template <class T>
  struct InfixCategory {
    Op op;
    std::optional<T> lhs;
    T rhs;
  };

  using Infix = InfixCategory<EXPR>;
  template <class T>
  struct ScheduleCategory {
    App expr;
    T time;
  };
  using Schedule = ScheduleCategory<EXPR>;
  struct EnvVar {
    std::string id;
  };

  template <class T, class ID>
  struct AssignmentProto {
    ID id;
    T expr;
  };
  using Id = typename ExprCommon<EXPR>::Id;
  using Assignment = AssignmentProto<EXPR, Id>;

  template <class T>
  using LCategory = typename ExprCommon<EXPR>::template LambdaCategory<T, Id>;

  using DefFn = AssignmentProto<LCategory<EXPR>, Id>;
  // empty placeholder that indicates the expression point itself
  struct RecVal {};
  using DefFnRec = AssignmentProto<LCategory<Either<EXPR, RecVal>>, Id>;

  struct Return {
    std::optional<EXPR> v;
  };

  using Statement = std::variant<Assignment, DefFn, DefFnRec, App, Schedule, Return>;

  //   using ExtFun;TODO
  struct Block {
    Aggregate<List, Statement> statements;
    std::optional<Return> ret;
  };
};

struct Hast {
  struct expr;
  using Expr = HastCommon<Box<expr>>;
  using LExpr = ExprCommon<Box<expr>>;
  using FloatLit = LExpr::FloatLit;
  using IntLit = LExpr::IntLit;
  using BoolLit = LExpr::BoolLit;
  using StringLit = LExpr::StringLit;
  using Symbol = LExpr::Symbol;
  using TupleLit = LExpr::TupleLit;
  using TupleGet = LExpr::TupleGet;
  using ArrayLit = LExpr::ArrayLit;
  using ArrayGet = LExpr::ArrayGet;
  using ArraySize = LExpr::ArraySize;
  using StructLit = LExpr::StructLit;
  using StructGet = LExpr::StructGet;
  using Lambda = LExpr::Lambda;
  using If = LExpr::If;

  using App = Expr::App;
  using Infix = Expr::Infix;
  using EnvVar = Expr::EnvVar;
  using Return = Expr::Return;
  using Block = Expr::Block;
  struct expr {
    using type = std::variant<FloatLit, IntLit, BoolLit, StringLit, Symbol, TupleLit, TupleGet,
                              ArrayLit, ArrayGet, ArraySize, StructLit, StructGet, Lambda, If, App,
                              App, Infix, EnvVar, Return, Block>;
    type v;
    operator type&() { return v; };        // NOLINT
    operator const type&() { return v; };  // NOLINT
  };
};

struct TopLevel {
  using expr = Hast::expr;
  // type name = hoge
  using TypeAlias = Alias_t;
  // import("")
  struct Import {
    std::string path;
  };

  using Statement = HastCommon<Box<expr>>::Statement;

  using CompilerDirective = std::variant<TypeAlias, Import>;

  using Expression = List<std::variant<Statement, CompilerDirective>>;
};

}  // namespace mimium