/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/type_infer_visitor.hpp"

namespace mimium {

using ITypePtr = TypeInferer::ITypePtr;

auto makeITypePtr = [](auto&& a) { return std::make_shared<IType::Value>(IType::Value{a}); };

//返り値を考慮しないvoid関数専用の簡易zipWith
template <typename T, typename U, typename F>
void zipWith(List<T>& a, List<U>& b, F&& lambda) {
  assert(a.size() == b.size());
  auto&& iter_a = std::begin(a);
  auto&& iter_b = std::begin(b);
  for (int i = 0; i < a.size(); i++) { lambda(*iter_a++, *iter_b++); }
}

template <typename T>
using boxenabler = std::enable_if_t<isBoxed(T())>;
// Box型やAliasの扱いなどが辛いがテンプレートを毎回書くのが辛いので、overloaded的にラムダと組み合わせる
template <typename... Ts>
struct UnifyVisitor : Ts... {
  TypeInferer& inferer;
  explicit UnifyVisitor(TypeInferer& i, Ts... ts) : Ts(ts)..., inferer(i){};
  using Ts::operator()...;
  template <class T>
  using isIntermediate = std::is_same<T, IType::Intermediate>;
  template <class T>
  using isAlias = std::is_same<T, IType::Alias>;

  template <class T>
  const static bool enablehelper = isIntermediate<T>::value || isAlias<T>::value;

  void operator()(IType::Intermediate& a, IType::Intermediate& b) {
    if (a.content) {
      auto btmp = IType::Value{b};
      inferer.unify(a.content.value(), btmp);
      b.content = btmp;
    } else if (b.content) {
      auto atmp = IType::Value{a};
      inferer.unify(atmp, b.content.value());
      a.content = atmp;
    } else {
      a.content = IType::Value{b};
    }
    // a.v.type_id = b.v.type_id;    //ポインタ自体も揃えてしまう
  }

  template <class T>
  auto operator()(IType::Intermediate& a, T& b)
      -> std::enable_if_t<!isIntermediate<T>::value, void> {
    IType::Value btmp{b};
    bool occur = TypeInferer::occurCheck(a, btmp);
    if (occur) { throw std::runtime_error("type loop detected"); }
    if (a.content) {
      inferer.unify(a.content.value(), btmp);
    } else {
      a.content = IType::Value{b};
    }
  }

  template <class T>
  auto operator()(T& a, IType::Intermediate& b)
      -> std::enable_if_t<!isIntermediate<T>::value, void> {
    (*this)(b, a);
  }
  void makeAliasIntermediate(IType::Alias& a){
    if (std::holds_alternative<IType::Unknown>(a.v.v.getraw().v)) {
      a.v.v = IType::Value{inferer.makeNewTypeVar(0)};
    }
  }
  void operator()(IType::Alias& a, IType::Alias& b) {
    makeAliasIntermediate(a);
    makeAliasIntermediate(b);
    inferer.unify(a.v.v, b.v.v);
  }

  template <class T>
  auto operator()(IType::Alias& a, T& b) -> std::enable_if_t<!isIntermediate<T>::value, void> {
    makeAliasIntermediate(a);
    IType::Value tmpb = IType::Value{b};
    inferer.unify(a.v.v, tmpb);
    b = std::get<T>(tmpb.v);
  }
  template <class T>
  auto operator()(T& a, IType::Alias& b) -> std::enable_if_t<!isIntermediate<T>::value, void> {
    (*this)(b, a);
  }
};
template <class I, class... Ts>
UnifyVisitor(I, Ts...) -> UnifyVisitor<Ts...>;

void TypeInferer::unify(IType::Value& a, IType::Value& b) {
  auto&& unifylambda = [&](IType::Value& a, IType::Value& b) -> void { unify(a, b); };
  UnifyVisitor vis{
      *this,
      [&](IType::Unknown& a, IType::Unknown& b) -> void {},
      [&](IType::Tuple& a, IType::Tuple& b) -> void { zipWith(a.v, b.v, unifylambda); },
      [&](IType::Variant& a, IType::Variant& b) -> void { zipWith(a.v, b.v, unifylambda); },
      [&](IType::Record& a, IType::Record& b) -> void {
        auto&& iter_a = std::begin(a.v);
        auto&& iter_b = std::begin(b.v);
        for (int i = 0; i < a.v.size(); i++) {
          if (iter_a->key != iter_b->key) { throw std::runtime_error("type check error"); }
          unify(iter_a->v, iter_b->v);
          std::advance(iter_a, 1);
          std::advance(iter_b, 1);
        }
      },
      [&](IType::Function& a, IType::Function& b) -> void {
        zipWith(a.v.first, b.v.first, unifylambda);
        unify(a.v.second, b.v.second);
      },
      [&](IType::Array& a, IType::Array& b) -> void {
        if (a.v.size != b.v.size) { throw std::runtime_error("array sizes are different"); }
        unify(a.v.v, b.v.v);
      },
      [&](IType::ListT& a, IType::ListT& b) -> void { unify(a.v, b.v); },
      [&](IType::Identified& a, IType::Identified& b) -> void {
        if (a.v.id != b.v.id) { throw std::runtime_error("identified types are different"); }
        unify(a.v.v, b.v.v);
      },
      [&](auto& a, auto& b) {
        if constexpr (std::is_same_v<decltype(a), decltype(b)>) {
          // do nothing
        } else {
          throw std::runtime_error("type check error");
        }
      }};

  std::visit(vis, a.v, b.v);
}

bool TypeInferer::occurCheck(IType::Intermediate const& lv, IType::Value& rv) {
  auto id = *lv.type_id;
  auto&& genmapper = [&](IType::Value& t) { return occurCheck(lv, t); };
  auto&& folder = [](bool a, bool b) { return a || b; };
  auto&& vis = overloaded_rec{
      [&](IType::Variant& t) { return foldl(fmap(t.v, genmapper), folder); },
      [&](IType::Tuple& t) { return foldl(fmap(t.v, genmapper), folder); },
      [&](IType::Function& t) {
        return foldl(fmap(t.v.first, genmapper), folder) || genmapper(t.v.second);
      },
      [&](IType::Array& t) { return genmapper(t.v.v); },
      [&](IType::Record& t) {
        return foldl(fmap(t.v, [&](IType::RecordKey& a) { return occurCheck(lv, a.v); }), folder);
      },
      [&](IType::Identified& t) { return genmapper(t.v.v); },
      [&](IType::ListT& t) { return genmapper(t.v); },
      [&](IType::Intermediate& t) {
        if (lv.level < t.level) { t.level = lv.level; }
        if (t.content) { return occurCheck(lv, t.content.value()); }
        return id == *t.type_id;
      },
      [&](IType::Alias& t) { return genmapper(t.v.v); },
      [&](auto& /*t*/) { return false; },
  };
  return std::visit(vis, rv.v);
}

template <class F, class... Ts>
struct GeneralVisitor : F, Ts... {
  using V = ITypePtr;
  using Ts::operator()...;

  template <class T>
  auto operator()(T const& a) -> std::enable_if_t<std::is_empty_v<std::decay_t<T>>, V> {
    return makeITypePtr(a);
  }

  template <class T, int id>
  V operator()(CategoryWrapped<List, id, T> const& a) {
    return makeITypePtr(CategoryWrapped<List, id, T>{
        fmap(a.v, [&](Box<IType::Value> const& v) { return Box(F::operator()(v.t)); })});
  }
  template <class T, int id>
  V operator()(CategoryWrapped<IdentCategory, id, T> const& a) {
    return makeITypePtr(a);
  }

  V operator()(IType::Array const& a) {
    auto arr = IType::Array{Box(IType::Value{}), a.v.size};
    arr.v.v.t = F::operator()(a.v.v.t);
    return makeITypePtr(std::move(arr));
  }
  template <int id>
  V operator()(CategoryWrapped<List, id, IType::RecordKey> const& a) {
    return makeITypePtr(
        CategoryWrapped<List, id, IType::RecordKey>{fmap(a.v, [&](IType::RecordKey const& a) {
          return IType::RecordKey{a.key, F::operator()(a.v.t)};
        })});
  }
  V operator()(IType::Function const& a) {
    return makeITypePtr(IType::Function{std::pair(
        fmap(a.v.first, [&](Box<IType::Value> const& v) { return Box(F::operator()(v.t)); }),
        Box(F::operator()(a.v.second.t)))});
  }
  V operator()(IType::Identified const& a) {
    return makeITypePtr(IType::Identified{a.v.id, F::operator()(a.v.v.t)});
  }
  V operator()(IType::Alias const& a) {
    return makeITypePtr(IType::Alias{a.v.id, F::operator()(a.v.v.t)});
  }
};

template <class F, class... Ts>
GeneralVisitor(F&& f, Ts&&... ts) -> GeneralVisitor<F, Ts...>;

TypeEnvH TypeInferer::infer(LAst::expr& expr) {
  auto env_i = std::make_shared<TypeEnvI>();
  auto toptype = inferInternal(expr, env_i, 0);
  return *substituteIntermediateVars(env_i, this->typevar_to_val);
}
template <typename... Ts>

struct SubstituteVisitor : Ts... {
  explicit SubstituteVisitor(Map<int, IType::Value> const& map, Ts... ts) : Ts(ts)..., map(map) {}
  Map<int, IType::Value> const& map;
};

HType::Value TypeInferer::lowerIType(IType::Value const& v,
                                     Map<IntPtr, IType::Value> const& typevar_map) {
  auto&& genmapper = [&](auto const& a) { return Box(lowerIType(a, typevar_map)); };
  auto&& recordmapper = [&](IType::RecordKey const& a) {
    return HType::RecordKey{a.key, genmapper(a.v)};
  };
  auto&& vis = overloaded{
      [](IType::Unit const& /*a*/) { return HType::Value{HType::Unit{}}; },
      [](IType::Float const& /*a*/) { return HType::Value{HType::Float{}}; },
      [](IType::Bool const& /*a*/) { return HType::Value{HType::Bool{}}; },
      [](IType::Int const& /*a*/) { return HType::Value{HType::Int{}}; },
      [](IType::String const& /*a*/) { return HType::Value{HType::String{}}; },
      [&](IType::Variant const& a) { return HType::Value{HType::Variant{fmap(a.v, genmapper)}}; },
      [&](IType::Tuple const& a) { return HType::Value{HType::Tuple{fmap(a.v, genmapper)}}; },
      [&](IType::Function const& a) {
        return HType::Value{
            HType::Function{std::pair(fmap(a.v.first, genmapper), genmapper(a.v.second))}};
      },
      [&](IType::Array const& a) {
        return HType::Value{HType::Array{genmapper(a.v.v), a.v.size}};
      },
      [&](IType::Record const& a) { return HType::Value{HType::Record{fmap(a.v, recordmapper)}}; },
      [&](IType::Identified const& a) {
        return HType::Value{HType::Identified{a.v.id, genmapper(a.v.v)}};
      },
      [&](IType::ListT const& a) { return HType::Value{HType::ListT{genmapper(a.v)}}; },
      [&](IType::Intermediate const& a) -> HType::Value {
        if (!a.content.has_value()) {
          throw std::runtime_error("failed to infer variable for typeid " +
                                   std::to_string(*a.type_id));
        }
        return genmapper(a.content.value());
      },
      [&](IType::TypeScheme const& a) {
        throw std::runtime_error("failed to instantiate type scheme for scheme-id" +
                                 std::to_string(*a.type_id));
        return HType::Value{HType::Unit{}};
      },
      [&](IType::Alias const& a) {
        return HType::Value{HType::Alias{a.v.id, genmapper(a.v.v)}};
      },
      [&](IType::Unknown const& /*a*/) {
        assert("Unknown Type detected.");
        return HType::Value{HType::Unit{}};
      },
  };
  return std::visit(vis, v.v);
}

std::shared_ptr<TypeEnvH> TypeInferer::substituteIntermediateVars(
    const std::shared_ptr<TypeEnvI> env, Map<IntPtr, IType::Value> const& typevar_map) {
  auto res = std::make_shared<TypeEnvH>();
  auto res_tmp = res;
  std::optional<std::shared_ptr<TypeEnvI>> e = env;
  while (e.has_value()) {
    for (const auto& [id, type] : e.value()->map) {
      res_tmp->map.emplace(id, std::make_shared<HType::Value>(lowerIType(*type, typevar_map)));
    }
    e = e.value()->child_env;
    if (e.has_value()) { res_tmp = res_tmp->expand(); }
  }
  return res;
}

IType::Intermediate TypeInferer::makeNewTypeVar(int level) {
  return IType::Intermediate{std::make_shared<int>(typevar_count++), level, std::nullopt};
}
IType::TypeScheme TypeInferer::makeNewTypeScheme() {
  return IType::TypeScheme{std::make_shared<int>(typevar_count++), 0};
}

ITypePtr TypeInferer::generalizeInternal(ITypePtr const& t, int level,
                                         Map<int, IntPtr>& typevar_to_scheme) {
  auto&& vis = GeneralVisitor{
      [&](ITypePtr const& a) -> ITypePtr { return generalize(a, level); },
      [&](IType::TypeScheme const& a) -> ITypePtr { return t; },
      [&](IType::Intermediate const& a) -> ITypePtr {
        if (a.content) { return generalize(a.content.value().t, level); }
        if (a.level > level) {
          auto tv_id = *a.type_id;
          if (typevar_to_scheme.count(tv_id) == 0) {
            auto newscheme = makeNewTypeScheme();
            typevar_to_scheme.emplace(tv_id, newscheme.type_id);
            return makeITypePtr(newscheme);
          }
          return makeITypePtr(IType::TypeScheme{typevar_to_scheme.at(tv_id)});
        }
        return t;
      }  // namespace mimium
  };
  return std::visit(vis, t->v);
}

ITypePtr TypeInferer::generalize(ITypePtr const& t, int level) {
  return generalizeInternal(t, level, this->typevar_to_scheme);
}

ITypePtr TypeInferer::instantiateInternal(ITypePtr const& t, int level,
                                          Map<int, IntPtr>& scheme_to_typevar) {
  auto&& vis = GeneralVisitor{[&](ITypePtr const& a) { return instantiate(a, level); },
                              [&](IType::Intermediate const& a) -> ITypePtr {
                                if (a.content) { return instantiate(a.content.value().t, level); }
                                return t;
                              },
                              [&](IType::TypeScheme const& a) -> ITypePtr {
                                auto scheme_id = *a.type_id;
                                if (scheme_to_typevar.count(scheme_id) == 0) {
                                  auto newtv = makeNewTypeVar(level);
                                  scheme_to_typevar.emplace(scheme_id, newtv.type_id);
                                }
                                return makeITypePtr(IType::Intermediate{
                                    scheme_to_typevar.at(scheme_id), level, std::nullopt});
                              }};
  return std::visit(vis, t->v);
}

ITypePtr TypeInferer::instantiate(ITypePtr const& t, int level) {
  return instantiateInternal(t, level, this->scheme_to_typevar);
}

ITypePtr TypeInferer::inferInternal(LAst::expr& expr, std::shared_ptr<TypeEnvI> env, int level) {
  auto&& inferlambda = [&](LAst::expr& e) { return Box(inferInternal(e, env, level)); };
  decltype(env) env_v = env;

  assert(env_v != nullptr);
  auto&& vis = overloaded{
      [&](LAst::FloatLit& /*a*/) -> ITypePtr { return makeITypePtr(IType::Float{}); },
      [&](LAst::IntLit& /*a*/) -> ITypePtr { return makeITypePtr(IType::Int{}); },
      [&](LAst::BoolLit& /*a*/) -> ITypePtr { return makeITypePtr(IType::Bool{}); },
      [&](LAst::StringLit& /*a*/) -> ITypePtr { return makeITypePtr(IType::String{}); },
      [&](LAst::TupleLit& a) -> ITypePtr {
        if (a.v.empty()) { return makeITypePtr(IType::Unit{}); }
        return makeITypePtr(IType::Tuple{fmap(a.v, inferlambda)});
      },
      [&](LAst::Symbol& a) -> ITypePtr {
        auto t = env->search(a.v.getUniqueName());
        if (!t.has_value()) {
          auto ext_iter = Intrinsic::ftable.find(a.v.v);
          if (ext_iter != Intrinsic::ftable.cend()) {
            return instantiate(std::make_shared<IType::Value>(ext_iter->second.mmmtype), level);
          }
          throw std::runtime_error("failed to look up symbol " + a.v.v);
        }
        return instantiate(t.value(), level);
      },

      [&](LAst::SelfLit& /*a*/) {
        return this->self_t_holder;  // TODO
      },
      [&](LAst::TupleGet& a) -> ITypePtr {
        auto ttype = inferlambda(a.v.expr).getraw();
        if (auto* tup = std::get_if<IType::Tuple>(&ttype.v)) {
          auto iter = std::next(tup->v.begin(), a.v.field);
          return iter->t;
        }
        throw std::runtime_error("type check error in Tuple getter");
      },
      [&](LAst::StructLit& a) -> ITypePtr {
        auto list = fmap(a.v, [&](LAst::StructKey& e) {
          return IType::RecordKey{e.key, inferlambda(e.v).getraw()};
        });
        return makeITypePtr(IType::Record{list});
      },
      [&](LAst::StructGet& a) -> ITypePtr {
        auto ttype = inferlambda(a.v.expr).getraw();
        if (auto* rec = std::get_if<IType::Record>(&ttype.v)) {
          auto iter = std::find_if(rec->v.begin(), rec->v.end(),
                                   [&](IType::RecordKey const& k) { return k.key == a.v.field; });
          if (iter != rec->v.end()) { return iter->v.t; }
        }
        throw std::runtime_error("type check error in Record Type getter");
      },
      [&](LAst::ArrayLit& a) -> ITypePtr {
        List<Box<IType::Value>> arglist = fmap(a.v, inferlambda);
        foldl(arglist, [&](IType::Value& a, IType::Value& b) {
          this->unify(a, b);
          return b;
        });
        return makeITypePtr(
            IType::Array{Box(*arglist.begin()->t), static_cast<int>(arglist.size())});
      },
      [&](LAst::ArrayGet& a) -> ITypePtr {
        auto ttype = inferlambda(a.v.expr).getraw();
        if (auto* rec = std::get_if<IType::Array>(&ttype.v)) { return rec->v.v.t; }
        throw std::runtime_error("type check error in Array Type getter");
      },
      [&](LAst::ArraySize& a) -> ITypePtr {
        inferlambda(a.v);
        return makeITypePtr(IType::Int{});
      },
      [&](LAst::Lambda& a) -> ITypePtr {
        auto atype = fmap(a.v.args, [&](LAst::Lvar const& a) -> Box<IType::Value> {
          return Box(a.type.value_or(IType::Value{makeNewTypeVar(level)}));
        });
        auto atype_iter = atype.cbegin();
        for (auto& a : a.v.args) {
          env->addToMap(a.id.getUniqueName(), atype_iter->t);
          atype_iter++;
        }
        auto body_t = inferInternal(a.v.body, env, level + 1);
        if (a.v.ret_type.has_value()) { unify(a.v.ret_type.value(), *body_t); }
        auto body_t_generic = generalize(body_t, level);
        {
          auto atype_iter = atype.begin();
          for (auto& a : a.v.args) {
            atype_iter->t = env->map.at(a.id.getUniqueName());
            atype_iter++;
          }
        }
        this->self_t_holder = body_t_generic;
        return makeITypePtr(IType::Function{std::pair(atype, Box(body_t_generic))});
      },
      [&](LAst::Sequence& a) -> ITypePtr {
        auto unit = IType::Value{IType::Unit{}};
        auto first_t = inferlambda(a.v.first);
        this->unify(first_t, unit);
        return inferlambda(a.v.second).t;
      },
      [&](LAst::NoOp& /*a*/) -> ITypePtr { return makeITypePtr(IType::Unit{}); },
      [&](LAst::Let& a) -> ITypePtr {
        auto lvtype = a.v.id.type.has_value() ? std::make_shared<IType::Value>(a.v.id.type.value())
                                              : makeITypePtr(makeNewTypeVar(level));
        env->addToMap(a.v.id.id.getUniqueName(), lvtype);
        auto exprtype = inferInternal(a.v.expr, env, level + 1);
        auto exprtype_general = generalize(exprtype, level);
        this->unify(*lvtype, *exprtype_general);
        auto rvaluet = inferInternal(a.v.body, env, level);
        return rvaluet;
      },
      [&](LAst::LetTuple& a) -> ITypePtr {
        List<Box<IType::Value>> argtype;
        for (auto&& arg : a.v.id) {
          Box<IType::Value> lvtype = arg.type.value_or(IType::Value{makeNewTypeVar(level)});
          env->addToMap(arg.id.getUniqueName(), lvtype.t);
          argtype.emplace_back(lvtype);
        }
        auto ttype = Box(IType::Value{IType::Tuple{argtype}});
        auto exprtype = inferInternal(a.v.expr, env, level + 1);
        unify(ttype, *exprtype);
        auto rvaluet = inferInternal(a.v.body, env, level);
        auto rvaluet_generic = generalize(rvaluet, level);
        return rvaluet_generic;
      },
      [&](LAst::App& a) -> ITypePtr {
        auto ftype = inferlambda(a.v.callee).getraw();
        auto argtypes = fmap(a.v.args, inferlambda);

        auto argproto = List<Box<IType::Value>>{};
        for (int i = 0; i < a.v.args.size(); i++) {
          argproto.emplace_back(IType::Value{makeNewTypeVar(level)});
        }
        auto ftypeproto =
            IType::Value{IType::Function{std::pair(argproto, IType::Value{makeNewTypeVar(level)})}};
        auto& argproto_ref = std::get<IType::Function>(ftypeproto.v).v.first;

        this->unify(ftypeproto, ftype);  // do this first before unify arguments
        zipWith(argproto_ref, argtypes, [&](auto& a, auto& b) { this->unify(a, b); });
        if (auto* ft_ptr = std::get_if<IType::Function>(&ftypeproto.v)) {
          return ft_ptr->v.second.t;
        }
        throw std::runtime_error("type check error in App");
      },
      [&](LAst::If& a) -> ITypePtr {
        auto condtype = inferlambda(a.v.cond);
        auto ftype = IType::Value{IType::Float{}};
        this->unify(condtype, ftype);
        auto thentype = inferlambda(a.v.vthen);
        if (a.v.velse.has_value()) {
          auto elsetype = inferlambda(a.v.velse.value());
          this->unify(thentype, elsetype);
          return thentype.t;
        }
        auto voidt = makeITypePtr(IType::Unit{});
        this->unify(thentype, *voidt);
        return voidt;
      },
      [&](LAst::Store& a) -> ITypePtr {
        auto voidt = makeITypePtr(IType::Unit{});
        auto id_t = env->search(a.id.getUniqueName());
        assert(id_t.has_value());
        auto expr_t = inferlambda(a.expr);
        this->unify(expr_t.getraw(), *id_t.value());
        return voidt;
      }};
  return std::visit(vis, expr.v);
}

}  // namespace mimium
