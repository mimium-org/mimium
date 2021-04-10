#pragma once

#include <list>
#include "abstractions.hpp"
#include "sexpr.hpp"
#include "type_new.hpp"

namespace mimium {
template <class T, class FieldT>
struct Getter {
  T expr;
  FieldT field;
};
template <class T, class Field, class F>
auto fmap(Getter<T, Field> const& v, F&& lambda) {
  return Getter<decltype(lambda(v.expr)), Field>{std::forward<F>(lambda)(v.expr), v.field};
}

template <class T, class ID>
struct LambdaCategory {
  List<ID> args;
  T body;
};
template <class T, class ID, class F>
auto fmap(LambdaCategory<T, ID> const& v, F&& lambda) -> decltype(auto) {
  using restype = std::invoke_result_t<F, T>;
  return LambdaCategory<restype, ID>{v.args, std::forward<F>(lambda)(v.body)};
}
// template <class T, class ID, class F>

// auto foldl(LambdaCategory<T, ID> const& v, F&& lambda) {
//   return lambda(foldl(v.args, std::forward<F>(lambda)), v.body);
// }

template <template <class...> class IdContainer, class ID, class T>
struct LetCategory {
  IdContainer<ID> id;
  T expr;
  T body;
};
template <template <class...> class IdContainer, class ID, class T, class F>
auto fmap(LetCategory<IdContainer, ID, T> const& v, F&& lambda) {
  return LetCategory<IdContainer, ID, decltype(lambda(v.expr))>{fmap(v.id, std::forward<F>(lambda)),
                                                                std::forward<F>(lambda)(v.expr),
                                                                std::forward<F>(lambda)(v.body)};
}

template <class T>
struct IfCategory {
  T cond;
  T vthen;
  std::optional<T> velse;
};
template <class T, class F>
auto fmap(IfCategory<T> const& v, F&& lambda) {
  return IfCategory<decltype(lambda(v.cond))>{std::forward<F>(lambda)(v.cond),
                                              std::forward<F>(lambda)(v.vthen),
                                              fmap(v.velse, std::forward<F>(lambda))};
}

template <class EXPR>
struct ExprCommon {
  template <template <class...> class Category, int ID = 0, class... T>
  using Aggregate = CategoryWrapped<Category, ID, T...>;

  template <template <class...> class Category, int ID = 0>
  using Wrapper = Aggregate<Category, ID, EXPR>;

  template <class T, int ID = 0>
  using Primitive = Aggregate<IdentCategory, ID, T>;

  template <class FieldT>
  auto toSExpr(Getter<EXPR, FieldT> const& v) {
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

  using FloatLit = Primitive<float>;
  using IntLit = Primitive<int>;
  using BoolLit = Primitive<bool>;
  using StringLit = Primitive<std::string>;

  struct SelfLit {};

  using Symbol = Primitive<std::string, 1>;

  using TupleLit = Wrapper<List, 0>;
  using TupleGet = Getter<EXPR, int>;

  using ArrayLit = Wrapper<List, 1>;
  using ArrayGet = Getter<EXPR, float>;
  using ArraySize = Wrapper<IdentCategory, 2>;

  using StructLit = Aggregate<List, 0, StructKey>;
  using StructGet = Getter<EXPR, std::string>;

  using Lambda = CategoryWrapped<LambdaCategory, 0, EXPR, Id>;

  using Let = LetCategory<IdentCategory, Id, EXPR>;
  using LetTuple = LetCategory<List, Id, EXPR>;

  using If = IfCategory<EXPR>;

  template <class T>
  struct AppCategory {
    EXPR callee;
    List<T> args;
  };

  using App = Wrapper<AppCategory>;
  static SExpr toSExpr(EXPR const& v) { return std::visit(sexpr_visitor, v.getraw().v); }
  constexpr static auto folder = [](auto&& a, auto&& b) { return cons(a, b); };
  inline const static auto mapper = [](EXPR const& e) -> SExpr { return toSExpr(e); };
  constexpr static auto listmatcher = [](std::string const& name, auto&& a) {
    return cons(makeSExpr(name), foldl(fmap(a.v, mapper), folder));
  };
  constexpr static auto structmapper = [](StructKey const& k) {
    return cons(makeSExpr(k.key), toSExpr(k.v));
  };
  constexpr static auto structmatcher = [](std::string const& name, auto&& a) {
    return cons(makeSExpr(name), foldl(fmap(a.v, structmapper), folder));
  };
  constexpr static auto gettermatcher = [](std::string const& name, auto&& a) {
    constexpr bool is_prim = std::is_same_v<int, std::decay_t<decltype(a.field)>>;
    std::string field;
    if constexpr (!is_prim) {
      field = a.field;
    } else {
      field = std::to_string(a.field);
    }
    return cons(makeSExpr(name), cons(toSExpr(a.expr), makeSExpr(field)));
  };
  inline const static auto sexpr_visitor = overloaded_rec{
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
      [](SelfLit const& /*a*/) { return makeSExpr("self"); },
      [](Symbol const& a) {
        return makeSExpr({"symbol", a.v});
      },
      [](TupleLit const& a) { return listmatcher("tuple", a); },
      [](TupleGet const& a) { return gettermatcher("tupleget", a); },
      [](ArrayLit const& a) { return listmatcher("array", a); },
      [](ArrayGet const& a) { return gettermatcher("arrayget", a); },
      [](ArraySize const& a) { return cons(makeSExpr("arraysize"), toSExpr(a.v)); },
      [](StructLit const& a) { return structmatcher("struct", a); },
      [](StructGet const& a) { return gettermatcher("structget", a); },
      [](Lambda const& a) {
        SExpr&& args = foldl(fmap<List>(a.v.args, [](Id&& a) { return makeSExpr(a.v); }), folder);
        return cons(makeSExpr("lambda"), cons(args, toSExpr(a.v.body)));
      },
      [](Let const& a) {
        return cons(makeSExpr({"let", a.id.v}), cons(toSExpr(a.expr), toSExpr(a.body)));
      },
      [](LetTuple const& a) {
        return cons(makeSExpr("let"), cons(toSExpr(a.expr), toSExpr(a.body)));
      },
      [](App const& a) {
        return cons(
            makeSExpr("app"),
            cons(toSExpr(a.v.callee), foldl(fmap(a.v.args, [](auto&& a) { return toSExpr(a); }),
                                            [](auto&& a, auto&& b) { return cons(a, b); })));
      },
      [](If const& a) {
        auto e = (a.velse) ? toSExpr(a.velse.value()) : makeSExpr("");
        return cons(makeSExpr("if"), cons(toSExpr(a.cond), cons(toSExpr(a.vthen), std::move(e))));
      },
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
  using SelfLit = Expr::SelfLit;
  using TupleLit = Expr::TupleLit;
  using TupleGet = Expr::TupleGet;
  using StructLit = Expr::StructLit;
  using StructGet = Expr::StructGet;
  using StructKey = Expr::StructKey;

  using ArrayLit = Expr::ArrayLit;
  using ArrayGet = Expr::ArrayGet;
  using ArraySize = Expr::ArraySize;
  using Lambda = Expr::Lambda;
  using Let = Expr::Let;
  using LetTuple = Expr::LetTuple;
  using App = Expr::App;

  using If = Expr::If;
  struct expr {
    using type = std::variant<FloatLit, IntLit, BoolLit, StringLit, SelfLit, Symbol, TupleLit,
                              TupleGet, StructLit, StructGet, ArrayLit, ArrayGet, ArraySize, Lambda,
                              Let, LetTuple, App, If>;
    type v;
    operator type&() { return v; };        // NOLINT
    operator const type&() { return v; };  // NOLINT
  };
};

// HIGH-LEVEL AST including Syntactic Sugar
template <class EXPR>
struct HastCommon {
  template <template <class...> class T, class U, int ID = 0>
  using Aggregate = typename ExprCommon<EXPR>::template Aggregate<T, ID, U>;

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

  using DefFn = AssignmentProto<LambdaCategory<EXPR, Id>, Id>;
  struct Return {
    std::optional<EXPR> v;
  };

  using Statement = std::variant<Assignment, DefFn, App, Schedule, Return>;

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
  using SelfLit = LExpr::SelfLit;
  using Symbol = LExpr::Symbol;
  using TupleLit = LExpr::TupleLit;
  using TupleGet = LExpr::TupleGet;
  using ArrayLit = LExpr::ArrayLit;
  using ArrayGet = LExpr::ArrayGet;
  using ArraySize = LExpr::ArraySize;
  using StructLit = LExpr::StructLit;
  using StructKey = LExpr::StructKey;
  using StructGet = LExpr::StructGet;

  using Lambda = LExpr::Lambda;
  using If = LExpr::If;

  using App = Expr::App;
  using Infix = Expr::Infix;
  using EnvVar = Expr::EnvVar;
  using Return = Expr::Return;
  using Block = Expr::Block;
  struct expr {
    using type = std::variant<FloatLit, IntLit, BoolLit, StringLit, SelfLit, Symbol, TupleLit,
                              TupleGet, ArrayLit, ArrayGet, ArraySize, StructLit, StructGet, Lambda,
                              If, App, Infix, EnvVar, Block>;
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