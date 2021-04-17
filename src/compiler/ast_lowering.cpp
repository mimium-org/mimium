#include "ast_lowering.hpp"
namespace mimium ::lowerast {

namespace {
using iter_t = List<Hast::Expr::Statement>::const_iterator;

constexpr std::string_view store_intrinsic = "store_intrinsic";
constexpr std::string_view schedule_intrinsic = "schedule_intrinsic";

}  // namespace

std::pair<AstLowerer::exprfunction, AstLowerer::EnvPtr> AstLowerer::lowerHStatement(
    Hast::Expr::Statement const& s, EnvPtr env) {
  const auto&& mE = [this](auto&& a) { return makeExpr(Box(std::move(a))); };

  using currytype = std::pair<exprfunction, EnvPtr>;
  // let a = A in let b = B in C... か Sequence(A,B,C)で繋いでいくことになる

  auto&& let_curried = [&](Hast::Id const& id, auto&& action_lower, EnvPtr env) -> currytype {
    auto newenv = env->expand();
    auto lexpr = action_lower();
    newenv->addToMap(id.v, lexpr);
    LAst::Id newid = {id.v, id.type, id.debuginfo};
    return std::pair(
        [&](LAst::expr&& b) -> LAst::expr {
          return mE(LAst::Let{{newid, lexpr, b}});
        },
        newenv);
  };
  auto&& seq_curried = [&](auto&& a, EnvPtr env) -> currytype {
    return std::pair([&](auto&& b) { return mE(LAst::Sequence{{std::pair(a, b)}}); }, env);
  };
  auto&& vis = overloaded{
      [&](Hast::Expr::Assignment const& a) -> currytype {
        const bool isnewvar = !env->existInLocal(a.v.id.v);
        if (isnewvar) {
          return let_curried(
              a.v.id, [&]() { return lowerHast(a.v.expr); }, env);
        }

        return seq_curried(mE(LAst::App{{mE(LAst::Symbol{{std::string(store_intrinsic)}}),
                                         std::list{Box(lowerHast(a.v.expr))}}}),
                           env);
      },
      [&](Hast::Expr::LetTuple const& a) -> currytype {
        int count = 0;
        LAst::expr lexpr = lowerHast(a.v.expr);
        auto newenv = env;
        for (auto&& id : a.v.id) {
          newenv = env->expand();
          auto texpr = LAst::TupleGet{{lexpr, count++}};
          newenv->addToMap(id.v, makeExpr(texpr));
        }
        List<LAst::Id> newid = fmap(a.v.id, [](Hast::Id const& i) {
          return LAst::Id{i.v, i.type, i.debuginfo};
        });
        return std::pair(
            [&](LAst::expr&& b) -> LAst::expr {
              return mE(LAst::LetTuple{{newid, lexpr, b}});
            },
            newenv);
      },
      [&](Hast::Expr::DefFn const& a) -> currytype {
        return let_curried(
            a.v.id, [&]() { return lowerHast(Hast::expr{a.v.expr}); }, env);
      },
      [&](Hast::Expr::App const& a) -> currytype {
        return seq_curried(Box(lowerHast(Hast::expr{a})), env);
      },
      [&](Hast::Expr::If const& a) -> currytype {
        return seq_curried(Box(lowerHast(Hast::expr{a})), env);
      },
      [&](Hast::Expr::Schedule const& a) -> currytype {
        return seq_curried(
            mE(LAst::App{{mE(LAst::Symbol{{std::string(schedule_intrinsic)}}),
                          {Box(lowerHast(Hast::expr{a.v.expr})), Box(lowerHast(a.v.time))}}}),
            env);
      }};
  return std::visit(vis, s);
}

LAst::expr AstLowerer::lowerHStatements(const List<Hast::Expr::Statement>& stmts) {
  auto&& process_state = [&](auto&& runner, auto&& stmts, auto&& env_init) {
    List<decltype(runner(*stmts.begin(), env_init).first)> res_container;
    auto env = env_init;
    for (auto&& s : stmts) {
      auto [getter, newenv] = runner(s, env);
      res_container.emplace_back(getter);
      env = newenv;
    }
    return res_container;
  };

  auto getters = process_state([this](auto&& a, auto&& e) { return lowerHStatement(a, e); }, stmts,
                               std::make_shared<Environment<Id, LAst::expr>>());
  auto res = foldl(getters, [](auto&& a, auto&& b) { return [&](auto&& c) { return a(b(c)); }; });
  return res(LAst::expr{LAst::NoOp{}});
}

LAst::expr AstLowerer::lowerHast(const Hast::expr& expr) {
  const auto&& mE = [this](auto&& a) { return makeExpr(Box(std::move(a))); };

  auto&& seqfolder = [&](LAst::expr const& a, LAst::expr const& b) {
    return LAst::expr{LAst::Sequence{{std::pair(a, b)}}};
  };
  auto genmapper = [&](auto&& a) { return Box(lowerHast(a)); };

  auto&& vis = overloaded{
      [&](Hast::FloatLit const& a) { return mE(LAst::FloatLit{{a.v}}); },
      [&](Hast::IntLit const& a) { return mE(LAst::IntLit{{a.v}}); },
      [&](Hast::StringLit const& a) { return mE(LAst::StringLit{{a.v}}); },
      [&](Hast::BoolLit const& a) { return mE(LAst::BoolLit{{a.v}}); },
      [&](Hast::SelfLit const& /**/) { return mE(LAst::SelfLit{}); },
      [&](Hast::Symbol const& a) { return mE(LAst::Symbol{{a.v}}); },
      [&](Hast::TupleLit const& a) { return mE(LAst::TupleLit{{fmap(a.v, genmapper)}}); },
      [&](Hast::TupleGet const& a) { return mE(LAst::TupleGet{{fmap(a.v, genmapper)}}); },
      [&](Hast::ArrayLit const& a) { return mE(LAst::ArrayLit{{fmap(a.v, genmapper)}}); },
      [&](Hast::ArrayGet const& a) {
        return mE(LAst::ArrayGet{{lowerHast(a.v.expr), lowerHast(a.v.field)}});
      },
      [&](Hast::ArraySize const& a) { return mE(LAst::ArraySize{{lowerHast(a.v)}}); },
      [&](Hast::StructLit const& a) {
        return mE(LAst::StructLit{{fmap<List>(a.v, [&](Hast::StructKey const& sa) {
          return LAst::StructKey{sa.key, lowerHast(sa.v)};
        })}});
      },
      [&](Hast::StructGet const& a) {
        return mE(LAst::StructGet{{lowerHast(a.v.expr), a.v.field}});
      },
      [&](Hast::Lambda const& a) {
        auto nargs = fmap(a.v.args, [](auto&& b) { return LAst::Expr::Id{b.v, b.type}; });
        return mE(LAst::Lambda{{nargs, lowerHast(a.v.body)}});
      },
      [&](Hast::If const& a) { return mE(LAst::If{{fmap(a.v, genmapper)}}); },
      [&](Hast::App const& a) {
        using Curry_t = Hast::Expr::CurryPlaceHolder;
        const bool is_partial =
            foldl(fmap(a.v.args, [](auto&& a) { return std::holds_alternative<Curry_t>(a); }),
                  [](bool a, bool b) { return a || b; });
        if (is_partial) {
          assert(false);  // TODO
        }
        return mE(LAst::App{{genmapper(a.v.callee), {fmap(a.v.args, [&](auto&& a) {
                               return Box(lowerHast(rv::get<Hast::expr>(a)));
                             })}}});
      },
      [&](Hast::Infix const& a) {
        std::optional<Box<LAst::expr>> lhs = fmap(a.v.lhs, genmapper);
        List<Box<LAst::expr>> args{genmapper(a.v.rhs)};
        if (lhs) { args.push_front(lhs.value()); }
        return mE(LAst::App{{mE(LAst::Symbol{{a.v.op.v}}), std::move(args)}});
      },
      [&](Hast::EnvVar const& a) {
        return mE(LAst::App{{mE(LAst::Symbol{{"getEnv"}}), {mE(LAst::StringLit{{a.id}})}}});
      },
      [&](Hast::Block const& a) -> LAst::expr {
        LAst::expr res = lowerHStatements(a.v.statements);
        if (a.v.ret) { return seqfolder(res, lowerHast(a.v.ret.value())); }
        return res;
      }};
  return std::visit(vis, expr.v);
}

LAst::expr removeSelf(LAst::expr const& expr) {
  // appの中にselfがあったらそのfnを変換する
  // expr(a,b,c...) -> expr(mem:type,a,b,c)
  // lambda(a,b,c)-> lambda(mem:type,a,b,c)
  // でもbeta変換しないとまだダメなのか？？関数をたどんないといけないもんな
  auto&& vis = overloaded{[&](LAst::FloatLit const& a) {},  [&](LAst::IntLit const& a) {},
                          [&](LAst::BoolLit const& a) {},   [&](LAst::StringLit const& a) {},
                          [&](LAst::SelfLit const& a) {},   [&](LAst::Symbol const& a) {},
                          [&](LAst::TupleLit const& a) {},  [&](LAst::TupleGet const& a) {},
                          [&](LAst::StructLit const& a) {}, [&](LAst::StructGet const& a) {},
                          [&](LAst::ArrayLit const& a) {},  [&](LAst::ArrayGet const& a) {},
                          [&](LAst::ArraySize const& a) {}, [&](LAst::Lambda const& a) {},
                          [&](LAst::Sequence const& a) {},  [&](LAst::NoOp const& a) {},
                          [&](LAst::Let const& a) {},       [&](LAst::LetTuple const& a) {},
                          [&](LAst::App const& a) {},       [&](LAst::If const& a) {}};
}

}  // namespace mimium::lowerast