#include "ast_lowering.hpp"
namespace mimium ::lowerast {

namespace {
using iter_t = List<Hast::Expr::Statement>::const_iterator;

constexpr std::string_view store_intrinsic = "store_intrinsic";
constexpr std::string_view schedule_intrinsic = "schedule_intrinsic";

}  // namespace

LAst::Lvar AstLowerer::lowerLvar(Hast::Lvar const& lv) {
  auto newid = Identifier{lv.id.v, makeUidforId()};
  return LAst::Lvar{
      newid,
      lv.type,
      lv.debuginfo,
  };
}
Identifier AstLowerer::makeId(std::string const& id) { return Identifier{id, makeUidforId()}; }
LAst::expr AstLowerer::lowerHStatement(Hast::Expr::Statement const& s,
                                       AstLowerer::stmt_iter_t const& next_s,
                                       AstLowerer::stmt_iter_t const& end_iter, EnvPtr env) {
  const auto mE = [this](auto&& a) { return LAst::expr{Box(std::move(a)), expr_uid++}; };
  std::function<LAst::expr(EnvPtr)> next_v_getter = [&](EnvPtr e) -> LAst::expr {
    if (next_s == end_iter) { return mE(LAst::NoOp{}); }
    return lowerHStatement(*next_s, std::next(next_s), end_iter, e);
  };
  // let a = A in let b = B in C... か Sequence(A,B,C)で繋いでいくことになる

  auto&& let_curried = [&](LAst::Lvar const& lv, Hast::expr const& expr, EnvPtr env) {
    auto lexpr = lowerHast(expr, env);

    env = env->expand();
    env->addToMap(lv.id.v, lv.id);
    return mE(LAst::Let{{lv, lexpr, next_v_getter(env)}});
  };
  auto&& seq_curried = [&](const auto& a, EnvPtr const env) -> LAst::expr {
    auto b = next_v_getter(env);
    if (std::holds_alternative<LAst::NoOp>(b.v)) { return a; }
    return mE(LAst::Sequence{{std::pair(a, b)}});
  };
  auto&& vis = overloaded{
      [&](Hast::Expr::Assignment const& a) {
        const bool isnewvar = !env->existInLocal(a.v.id.id.v);
        if (isnewvar) {
          auto lv = lowerLvar(a.v.id);
          return let_curried(lv, a.v.expr, env);
        }
        auto stored_v = lowerHast(a.v.expr, env);
        auto id = env->search(a.v.id.id.v);
        return seq_curried(Box(mE(LAst::Store{id.value(), stored_v})), env);
      },
      [&](Hast::Expr::LetTuple const& a) {
        int count = 0;
        LAst::expr lexpr = lowerHast(a.v.expr, env);
        auto newenv = env->expand();
        List<LAst::Lvar> newlvs = {};
        for (auto&& lv : a.v.id) {
          auto texpr = LAst::TupleGet{{lexpr, count++}};
          auto newlv = lowerLvar(lv);
          newenv->addToMap(lv.id.v, newlv.id);
          newlvs.emplace_back(newlv);
        }
        // auto newids = fmap(a.v.id, [&](Hast::Lvar const& i) { return lowerLvar(i); });
        return mE(LAst::LetTuple{{newlvs, lexpr, next_v_getter(newenv)}});
      },
      [&](Hast::Expr::DefFn const& a) {
        return let_curried(lowerLvar(a.v.id), Hast::expr{a.v.expr}, env);
      },
      [&](Hast::Expr::App const& a) {
        return seq_curried(Box(lowerHast(Hast::expr{a}, env)), env);
      },
      [&](Hast::Expr::If const& a) { return seq_curried(Box(lowerHast(Hast::expr{a}, env)), env); },
      [&](Hast::Expr::Schedule const& a) {
        return seq_curried(mE(LAst::App{{mE(LAst::Symbol{{std::string(schedule_intrinsic)}}),
                                         {Box(lowerHast(Hast::expr{a.v.expr}, env)),
                                          Box(lowerHast(a.v.time, env))}}}),
                           env);
      },
      [&](Hast::Expr::Return const& a) { return lowerHast(a.v.value(), env); }};
  return std::visit(vis, s);
}

LAst::expr AstLowerer::lowerHStatements(const List<Hast::Expr::Statement>& stmts,
                                        AstLowerer::EnvPtr env) {
  // List<exprfunction> res_container;
  auto first_elem = std::cbegin(stmts);
  auto res = lowerHStatement(*first_elem, std::next(first_elem), std::cend(stmts), env);
  if (std::holds_alternative<LAst::Sequence>(res.v)) {
    auto& seq = std::get<LAst::Sequence>(res.v);
    if (std::holds_alternative<LAst::NoOp>(seq.v.second.getraw().v)) { return seq.v.first; }
  }
  // if (std::holds_alternative<LAst::Let>(res.v)) {
  //   auto& let = std::get<LAst::Let>(res.v);
  //   assert(std::holds_alternative<LAst::NoOp>(let.v.body.getraw().v));
  //   if (opt_ret.has_value()) { let.v.body = lowerHast(opt_ret.value(), env); }
  // }
  // if (std::holds_alternative<LAst::LetTuple>(res.v)) {
  //   auto& let = std::get<LAst::LetTuple>(res.v);
  //   assert(std::holds_alternative<LAst::NoOp>(let.v.body.getraw().v));
  //   if (opt_ret.has_value()) { let.v.body = lowerHast(opt_ret.value(), env); }
  // }
  return res;
}

LAst::expr AstLowerer::lowerHast(const Hast::expr& expr, AstLowerer::EnvPtr& env) {
  const auto&& mE = [this](auto&& a) { return LAst::expr{Box(std::move(a)), expr_uid++}; };
  auto genmapper = [&](auto&& a) { return Box(lowerHast(a, env)); };

  auto&& vis = overloaded{
      [&](Hast::FloatLit const& a) { return mE(LAst::FloatLit{{a.v}}); },
      [&](Hast::IntLit const& a) { return mE(LAst::IntLit{{a.v}}); },
      [&](Hast::StringLit const& a) { return mE(LAst::StringLit{{a.v}}); },
      [&](Hast::BoolLit const& a) { return mE(LAst::BoolLit{{a.v}}); },
      [&](Hast::SelfLit const& /**/) { return mE(LAst::SelfLit{}); },
      [&](Hast::Symbol const& a) {
        auto newid = env->search(a.v.v);
        if (newid.has_value()) { return mE(LAst::Symbol{{newid.value()}}); }
        return mE(LAst::Symbol{{a.v.v}});  // in case of external symbol
      },
      [&](Hast::TupleLit const& a) { return mE(LAst::TupleLit{{fmap(a.v, genmapper)}}); },
      [&](Hast::TupleGet const& a) { return mE(LAst::TupleGet{{fmap(a.v, genmapper)}}); },
      [&](Hast::ArrayLit const& a) { return mE(LAst::ArrayLit{{fmap(a.v, genmapper)}}); },
      [&](Hast::ArrayGet const& a) {
        return mE(LAst::ArrayGet{{lowerHast(a.v.expr, env), lowerHast(a.v.field, env)}});
      },
      [&](Hast::ArraySize const& a) { return mE(LAst::ArraySize{{lowerHast(a.v, env)}}); },
      [&](Hast::StructLit const& a) {
        return mE(LAst::StructLit{{fmap<List>(a.v, [&](Hast::StructKey const& sa) {
          return LAst::StructKey{sa.key, lowerHast(sa.v, env)};
        })}});
      },
      [&](Hast::StructGet const& a) {
        return mE(LAst::StructGet{{lowerHast(a.v.expr, env), a.v.field}});
      },
      [&](Hast::Lambda const& a) {
        env = env->expand();
        auto nargs = fmap(a.v.args, [&](auto&& b) {
          auto newlv = lowerLvar(b);
          env->addToMap(newlv.id.v, newlv.id);
          return newlv;
        });
        auto body = lowerHast(a.v.body, env);
        env = env->parent_env.value();
        return mE(LAst::Lambda{{nargs, body}});
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
                               return Box(lowerHast(rv::get<Hast::expr>(a), env));
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
        auto stmts = a.v.statements;
        if (a.v.ret) { stmts.emplace_back(Box(Hast::Statement{Hast::Return{{a.v.ret}}})); }
        LAst::expr res = lowerHStatements(stmts, env);
        return res;
      }};
  return std::visit(vis, expr.v);
}

// LAst::expr removeSelf(LAst::expr const& expr) {
//   // appの中にselfがあったらそのfnを変換する
//   // expr(a,b,c...) -> expr(mem:type,a,b,c)
//   // lambda(a,b,c)-> lambda(mem:type,a,b,c)
//   // でもbeta変換しないとまだダメなのか？？関数をたどんないといけないもんな
//   auto&& vis = overloaded{[&](LAst::FloatLit const& a) {},  [&](LAst::IntLit const& a) {},
//                           [&](LAst::BoolLit const& a) {},   [&](LAst::StringLit const& a) {},
//                           [&](LAst::SelfLit const& a) {},   [&](LAst::Symbol const& a) {},
//                           [&](LAst::TupleLit const& a) {},  [&](LAst::TupleGet const& a) {},
//                           [&](LAst::StructLit const& a) {}, [&](LAst::StructGet const& a) {},
//                           [&](LAst::ArrayLit const& a) {},  [&](LAst::ArrayGet const& a) {},
//                           [&](LAst::ArraySize const& a) {}, [&](LAst::Lambda const& a) {},
//                           [&](LAst::Sequence const& a) {},  [&](LAst::NoOp const& a) {},
//                           [&](LAst::Let const& a) {},       [&](LAst::LetTuple const& a) {},
//                           [&](LAst::App const& a) {},       [&](LAst::If const& a) {}};
// }

}  // namespace mimium::lowerast