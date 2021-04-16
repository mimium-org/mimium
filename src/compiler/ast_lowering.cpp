#include "ast_lowering.hpp"
namespace mimium ::lowerast {

namespace {
using iter_t = List<Hast::Expr::Statement>::const_iterator;

constexpr std::string_view store_intrinsic = "store_intrinsic";
constexpr std::string_view schedule_intrinsic = "schedule_intrinsic";

}  // namespace

std::pair<AstLowerer::exprfunction, AstLowerer::EnvPtr> AstLowerer::lowerHStatement(
    Hast::Expr::Statement const& s, EnvPtr env) {
  const auto&& mE = [this](auto&& a) { return makeExpr(a); };

  using currytype = std::pair<exprfunction, EnvPtr>;
  // let a = A in let b = B in C... か Sequence(A,B,C)で繋いでいくことになる

  auto&& let_curried = [&](auto&& id, auto&& action_lower, EnvPtr env) -> currytype {
    auto newenv = env->expand();
    auto lexpr = action_lower();
    newenv->addToMap(id.v, lexpr);
    return std::pair([&](auto&& b) { return mE(LAst::Let{{id.v}, lexpr, b}); }, newenv);
  };
  auto&& seq_curried = [&](auto&& a, EnvPtr env) -> currytype {
    return std::pair([&](auto&& b) { return mE(LAst::Sequence{{a, b}}); }, env);
  };
  auto&& vis =
      overloaded{[&](Hast::Expr::Assignment const& a) -> currytype {
                   const bool isnewvar = !env->existInLocal(a.id.v);
                   if (isnewvar) {
                     return let_curried(
                         a.id, [&]() { return lowerHast(a.expr); }, env);
                   }

                   return seq_curried(mE(LAst::App{mE(LAst::Symbol{std::string(store_intrinsic)}),
                                                   std::list{Box(lowerHast(a.expr))}}),
                                      env);
                 },
                 [&](Hast::Expr::DefFn const& a) -> currytype {
                   return let_curried(
                       a.id, [&]() { return lowerHast(Hast::expr{a.expr}); }, env);
                 },
                 [&](Hast::Expr::App const& a) -> currytype {
                   return seq_curried(Box(lowerHast(Hast::expr{a})), env);
                 },
                 [&](Hast::Expr::Schedule const& a) -> currytype {
                   return seq_curried(
                       mE(LAst::App{mE(LAst::Symbol{std::string(schedule_intrinsic)}),
                                    {Box(lowerHast(Hast::expr{a.expr})), Box(lowerHast(a.time))}}),
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
  const auto&& mE = [this](auto&& a) { return makeExpr(a); };

  auto&& seqfolder = [&](LAst::expr const& a, LAst::expr const& b) {
    return LAst::Sequence{{a, b}};
  };
  auto genmapper = [&](auto&& a) { return Box(lowerHast(a)); };

  auto&& vis = overloaded{
      [&](Hast::FloatLit const& a) { return mE(LAst::FloatLit{a.v}); },
      [&](Hast::IntLit const& a) { return mE(LAst::IntLit{a.v}); },
      [&](Hast::StringLit const& a) { return mE(LAst::StringLit{a.v}); },
      [&](Hast::BoolLit const& a) { return mE(LAst::BoolLit{a.v}); },
      [&](Hast::SelfLit const& /**/) { return mE(LAst::SelfLit{}); },
      [&](Hast::Symbol const& a) { return mE(LAst::Symbol{a.v}); },
      [&](Hast::TupleLit const& a) { return mE(LAst::TupleLit{fmap(a.v, genmapper)}); },
      [&](Hast::TupleGet const& a) { return mE(fmap(a, genmapper)); },
      [&](Hast::ArrayLit const& a) { return mE(LAst::ArrayLit{fmap(a.v, genmapper)}); },
      [&](Hast::ArrayGet const& a) { return mE(fmap(a, genmapper)); },
      [&](Hast::ArraySize const& a) { return mE(LAst::ArraySize{lowerHast(a.v)}); },
      [&](Hast::StructLit const& a) {
        return mE(LAst::StructLit{fmap(a.v, [&](Hast::StructKey&& sa) {
          return LAst::StructKey{sa.key, lowerHast(sa.v)};
        })});
      },
      [&](Hast::StructGet const& a) {
        return mE(LAst::StructGet{lowerHast(a.expr), a.field});
      },
      [&](Hast::Lambda const& a) {
        auto nargs = fmap(a.v.args, [](auto&& b) { return LAst::Expr::Id{b.v, b.type}; });
        return mE(LAst::Lambda{nargs, lowerHast(a.v.body)});
      },
      [&](Hast::If const& a) { return mE(fmap(a, genmapper)); },
      [&](Hast::App const& a) {
        using Curry_t = Hast::Expr::CurryPlaceHolder;
        const bool is_partial =
            foldl(fmap(a.args, [](auto&& a) { return std::holds_alternative<Curry_t>(a); }),
                  [](bool a, bool b) { return a || b; });
        if (is_partial) {
          assert(false);  // TODO
        }
        return mE(LAst::App{genmapper(a.callee), {fmap(a.args, [&](auto&& a) {
                              return Box(lowerHast(rv::get<Hast::expr>(a)));
                            })}});
      },
      [&](Hast::Infix const& a) {
        std::optional<Box<LAst::expr>> lhs = fmap(a.lhs, genmapper);
        List<Box<LAst::expr>> args{genmapper(a.rhs)};
        if (lhs) { args.push_front(lhs.value()); }
        return mE(LAst::App{mE(LAst::Symbol{a.op.v}), std::move(args)});
      },
      [&](Hast::EnvVar const& a) {
        return mE(LAst::App{mE(LAst::Symbol{"getEnv"}), {mE(LAst::StringLit{a.id})}});
      },
      [&](Hast::Block const& a) {
        LAst::expr res = lowerHStatements(a.statements.v);
        if (a.ret) { return mE(seqfolder(res, lowerHast(a.ret.value()))); }
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