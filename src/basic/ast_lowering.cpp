#include "ast_lowering.hpp"

namespace mimium ::lowerast {

LAst::expr lowerHStatement(const Hast::Expr::Statement s) {
  auto&& vis = overloaded{[&](Hast::Expr::Assignment const& a) { return LAst::expr{}; },
                          [&](Hast::Expr::DefFn const& a) { return LAst::expr{}; },
                          [&](Hast::Expr::App const& a) { return LAst::expr{}; },
                          [&](Hast::Expr::Schedule const& a) { return LAst::expr{}; },
                          [&](Hast::Expr::Return const& a) { return LAst::expr{}; }};
  return std::visit(vis, s);
}

LAst::expr lowerHast(const Hast::expr& expr, TypeEnv const& env) {
  auto&& mE = [](auto&& a) { return Box(LAst::expr{a}); };
  auto type = env.map.find(expr);

  auto&& seqfolder = [&](LAst::expr const& a, LAst::expr const& b) {
    return LAst::App{mE(LAst::Symbol{"Seq"}), {a, b}};
  };
  auto genmapper = [&](auto&& a) { return Box(lowerHast(a, env)); };

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
      [&](Hast::ArraySize const& a) { return mE(LAst::ArraySize{lowerHast(a.v, env)}); },
      [&](Hast::StructLit const& a) {
        return mE(LAst::StructLit{fmap(a.v, [&](Hast::StructKey&& sa) {
          return LAst::StructKey{sa.key, lowerHast(sa.v, env)};
        })});
      },
      [&](Hast::StructGet const& a) {
        return mE(LAst::StructGet{lowerHast(a.expr, env), a.field});
      },
      [&](Hast::Lambda const& a) {
        auto nargs = fmap(a.v.args, [](auto&& b) { return LAst::Expr::Id{b.v, b.type}; });
        return mE(LAst::Lambda{nargs, lowerHast(a.v.body, env)});
      },
      [&](Hast::If const& a) { return mE(fmap(a, genmapper)); },
      [&](Hast::App const& a) {
        auto&& check = [](auto&& a) {
          return std::holds_alternative<Hast::Expr::CurryPlaceHolder>(a);
        };
        auto list_isplaceholder = fmap(a.args, [&](auto&& a) { return check(a); });
        const bool is_partial = foldl(list_isplaceholder, [](bool a, bool b) { return a || b; });
        if (is_partial) {
          assert(false);  // TODO
        }
        return mE(LAst::App{genmapper(a.callee), {fmap(a.args, [&](auto&& a) {
                              return Box(lowerHast(rv::get<Hast::expr>(a), env));
                            })}});
      },
      [&](Hast::Infix const& a) {
        std::optional<Box<LAst::expr>> lhs = fmap(a.lhs, genmapper);
        List<Box<LAst::expr>> args{Box(genmapper(a.rhs))};
        if (lhs) { args.push_front(lhs.value()); }
        return mE(LAst::App{mE(LAst::Symbol{"getEnv"}), std::move(args)});
      },
      [&](Hast::EnvVar const& a) {
        return mE(LAst::App{mE(LAst::Symbol{"getEnv"}), {mE(LAst::StringLit{a.id})}});
      },
      [&](Hast::Block const& a) {
        LAst::expr res = mE(LAst::App{
            mE(LAst::Symbol{"Seq"}),
            {foldl(fmap(a.statements, [&](auto&& a) { return lowerHStatement(a); }), seqfolder)}});
        if (a.ret) {
          return mE(LAst::App{mE(LAst::Symbol{"Seq"}), {res, lowerHStatement(a.ret.value())}});
        }
        return Box(res);
      },
      [&](const auto& a) { return mE(LAst::StringLit{""}); }};
  return std::visit(vis, expr.v);
}
}  // namespace mimium::lowerast