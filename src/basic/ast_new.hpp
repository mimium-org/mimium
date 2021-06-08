#pragma once

#include <list>
#include "abstractions.hpp"
#include "debuginfo.hpp"
#include "sexpr.hpp"
#include "type_new.hpp"

namespace mimium {

using mmmfloat = double;

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
  using restype = decltype(lambda(v.body));
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
  struct CategoryWrappedExpr : public CategoryWrapped<Category, ID, T...> {
    DebugInfo debuginfo;
  };
  template <template <class...> class Category, int ID = 0, class... T>
  using Aggregate = CategoryWrappedExpr<Category, ID, T...>;

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
    std::optional<IType::Value> type;
    DebugInfo debuginfo;
  };
  struct StructKey {
    std::string key;
    EXPR v;
  };

  using FloatLit = Primitive<mmmfloat>;
  using IntLit = Primitive<int>;
  using BoolLit = Primitive<bool>;
  using StringLit = Primitive<std::string>;

  struct SelfLit {
    DebugInfo debuginfo;
  };

  using Symbol = Primitive<std::string, 1>;

  using TupleLit = Wrapper<List, 0>;
  using TupleGet = Aggregate<Getter, 0, EXPR, int>;

  using ArrayLit = Wrapper<List, 1>;
  using ArrayGet = Aggregate<Getter, 0, EXPR, EXPR>;
  using ArraySize = Wrapper<IdentCategory, 2>;
  using StructLit = Aggregate<List, 0, StructKey>;
  using StructGet = Aggregate<Getter, 0, EXPR, std::string>;

  using Lambda = CategoryWrappedExpr<LambdaCategory, 0, EXPR, Id>;

  // pair of expressions which is used to express a sequence of statements.
  // the first value should be void(unit)-type Application of function.
  struct NoOp {};
  using Sequence = CategoryWrappedExpr<Pair, 0, EXPR>;

  template <class T, class ID>
  using LetCategoryIdent = LetCategory<IdentCategory, T, ID>;
  template <class T, class ID>
  using LetCategoryList = LetCategory<List, T, ID>;

  using Let = CategoryWrappedExpr<LetCategoryIdent, 0, Id, EXPR>;
  using LetTuple = CategoryWrappedExpr<LetCategoryList, 0, Id, EXPR>;

  using If = CategoryWrappedExpr<IfCategory, 0, EXPR>;

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
    constexpr bool is_prim = std::is_same_v<int, std::decay_t<decltype(a.v.field)>>;
    constexpr bool is_expr = std::is_same_v<EXPR, std::decay_t<decltype(a.v.field)>>;
    if constexpr (is_expr) {
      return cons(makeSExpr(name), cons(toSExpr(a.v.expr), toSExpr(a.v.field)));
    } else {
      std::string field;
      if constexpr (!is_prim) {
        field = a.v.field;
      } else {
        field = std::to_string(a.v.field);
      }
      return cons(makeSExpr(name), cons(toSExpr(a.v.expr), makeSExpr(field)));
    }
  };
  inline const static auto sexpr_visitor = overloaded{
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
        SExpr&& args = foldl(fmap(a.v.args, [](Id const& a) { return makeSExpr(a.v); }), folder);
        return cons(makeSExpr("lambda"), cons(args, toSExpr(a.v.body)));
      },
      [](NoOp const& /**/) { return makeSExpr("noop"); },
      [](Sequence const& a) {
        return cons(makeSExpr("sequence"), cons(toSExpr(a.v.first), toSExpr(a.v.second)));
      },
      [](Let const& a) {
        return cons(makeSExpr({"let", a.v.id.v}), cons(toSExpr(a.v.expr), toSExpr(a.v.body)));
      },
      [](LetTuple const& a) {
        return cons(makeSExpr({"lettuple"}),
                    cons(foldl(fmap(a.v.id, [](Id const& a) { return makeSExpr(a.v); }), folder),
                         cons(toSExpr(a.v.expr), toSExpr(a.v.body))));
      },
      [](App const& a) {
        return cons(makeSExpr("app"),
                    cons(toSExpr(a.v.callee), foldl(fmap(a.v.args, mapper), [](auto&& a, auto&& b) {
                           return cons(a, b);
                         })));
      },
      [](If const& a) {
        auto e = (a.v.velse) ? toSExpr(a.v.velse.value()) : makeSExpr("");
        return cons(makeSExpr("if"),
                    cons(toSExpr(a.v.cond), cons(toSExpr(a.v.vthen), std::move(e))));
      },
      [](auto const& a) { return makeSExpr(""); }};
  static std::string toString(EXPR const& v) { 
    SExpr se = toSExpr(v);
    return mimium::toString(se); }
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
  using Id = Expr::Id;
  using Lambda = Expr::Lambda;
  using Sequence = Expr::Sequence;
  using NoOp = Expr::NoOp;

  using Let = Expr::Let;
  using LetTuple = Expr::LetTuple;
  using App = Expr::App;

  using If = Expr::If;
  struct expr {
    using type = std::variant<FloatLit, IntLit, BoolLit, StringLit, SelfLit, Symbol, TupleLit,
                              TupleGet, StructLit, StructGet, ArrayLit, ArrayGet, ArraySize, Lambda,
                              Sequence, NoOp, Let, LetTuple, App, If>;
    type v;
    int uniqueid;
    operator type&() { return v; };        // NOLINT
    operator const type&() { return v; };  // NOLINT
  };
};

// HIGH-LEVEL AST including Syntactic Sugar
template <class EXPR>
struct HastCommon {
  template <template <class...> class C, int ID = 0, class... T>
  using Aggregate = typename ExprCommon<EXPR>::template Aggregate<C, ID, T...>;

  struct CurryPlaceHolder {};
  using CurryArg = std::variant<EXPR, CurryPlaceHolder>;
  template <class T>
  using AppCategory = typename ExprCommon<EXPR>::template AppCategory<T>;
  using App = Aggregate<AppCategory, 0, CurryArg>;

  struct Op {
    std::string v;
  };
  template <class T>
  struct InfixCategory {
    Op op;
    std::optional<T> lhs;
    T rhs;
  };

  using Infix = Aggregate<InfixCategory, 0, EXPR>;
  template <class T>
  struct ScheduleCategory {
    App expr;
    T time;
  };
  using Schedule = Aggregate<ScheduleCategory, 0, EXPR>;
  struct EnvVar {
    std::string id;
  };

  template <class T, class ID>
  struct AssignmentProto {
    ID id;
    T expr;
  };
  using Id = typename ExprCommon<EXPR>::Id;
  using Assignment = Aggregate<AssignmentProto, 0, EXPR, Id>;
  using LetTuple = Aggregate<AssignmentProto, 0, EXPR, List<Id>>;

  using Lambda = typename ExprCommon<EXPR>::Lambda;
  using DefFn = Aggregate<AssignmentProto, 0, Lambda, Id>;
  using Return = Aggregate<std::optional, 0, EXPR>;

  using If = typename ExprCommon<EXPR>::If;
  using Statement = std::variant<Assignment, LetTuple, DefFn, App, If, Schedule>;
  using Statements = Aggregate<List, 0, Statement>;
  //   using ExtFun;TODO
  template <class T, class U>
  struct BlockCategory {
    List<T> statements;
    std::optional<U> ret;
  };
  using Block = Aggregate<BlockCategory, 0, Statement, EXPR>;
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
  using Id = LExpr::Id;
  using Lambda = LExpr::Lambda;
  using If = LExpr::If;

  using App = Expr::App;
  using CurryArg = Expr::CurryArg;
  using Infix = Expr::Infix;
  using Op = Expr::Op;
  using EnvVar = Expr::EnvVar;
  using Return = Expr::Return;
  using Block = Expr::Block;
  using DefFn = Expr::DefFn;
  using Assignment = Expr::Assignment;
  using LetTuple = Expr::LetTuple;
  using Schedule = Expr::Schedule;
  using Statement = Expr::Statement;
  using Statements = Expr::Statements;

  struct expr {
    using type = std::variant<FloatLit, IntLit, BoolLit, StringLit, SelfLit, Symbol, TupleLit,
                              TupleGet, ArrayLit, ArrayGet, ArraySize, StructLit, StructGet, Lambda,
                              If, App, Infix, EnvVar, Block>;
    type v;
    operator type&() { return v; };        // NOLINT
    operator const type&() { return v; };  // NOLINT
  };

  template <class LOCATION>
  static auto processReturn(Statements& s, LOCATION const& loc) {
    const auto& lastline = s.v.back();
    std::optional<Box<expr>> optexpr = std::nullopt;
    if (std::holds_alternative<App>(lastline)) {
      optexpr = expr{std::get<App>(lastline)};
      s.v.pop_back();
    } else if (std::holds_alternative<If>(lastline)) {
      optexpr = expr{std::get<If>(lastline)};
      s.v.pop_back();
    }
    return Block{std::move(s).v, std::move(optexpr), {loc, "block"}};
  }
};

struct TopLevel {
  using expr = Hast::expr;
  // type name = hoge
  using TypeAlias = Pair<std::string, IType::Value>;
  using AliasMap_t = Map<std::string, IType::Value>;
  // import("")
  struct Import {
    std::string path;
  };

  // using Statement = Hast::Statement;

  using CompilerDirective = std::variant<TypeAlias, Import>;
  using Statement = std::variant<Hast::Statement, CompilerDirective>;
  using Expression = List<Statement>;
};

template <class T>
static std::string toString(typename T::expr const& e) {
  auto be = Box<typename T::expr>(e);
  return ExprCommon<Box<typename T::expr>>::toString(be);
}

}  // namespace mimium