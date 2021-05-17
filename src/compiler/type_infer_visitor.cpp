/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/type_infer_visitor.hpp"

namespace mimium {
//返り値を考慮しないvoid関数専用の簡易zipWith
template <typename T, typename F>
void zipWith(List<T>& a, List<T>& b, F&& lambda) {
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

  template <typename T, class U>
  void operator()(T& a, U& b) {
    if constexpr (is_boxed_t<T>::value && is_boxed_t<T>::value) {
      (*this)(a.getraw(), b.getraw());
    } else if constexpr (is_boxed_t<T>::value) {
      (*this)(a.getraw(), b);
    } else if constexpr (is_boxed_t<U>::value) {
      (*this)(a, b.getraw());
    } else if constexpr (std::is_same_v<T, U>) {
      // do nothing
    } else if constexpr (isAlias<T>::value && isAlias<U>::value) {
      inferer.unify(a.v.v, b.v.v);
    } else if constexpr (isAlias<T>::value) {
      IType::Value tmpb = IType::Value{b};
      inferer.unify(a.v.v, tmpb);
      b = std::get<U>(tmpb.v);
    } else if constexpr (isAlias<U>::value) {
      IType::Value tmpa = IType::Value{a};
      inferer.unify(tmpa, b.v.v);
      a = std::get<T>(tmpa.v);
    } else if constexpr (isIntermediate<T>::value && isIntermediate<U>::value) {
      *a.v.type_id = *b.v.type_id;  //ポインタの中身を合わせる
      a.v.type_id = b.v.type_id;    //ポインタ自体も揃えてしまう
    } else if constexpr (isIntermediate<T>::value) {
      IType::Value btmp{b};
      bool occur = TypeInferer::occurCheck(a, btmp);
      if (!occur) { inferer.typevar_to_val.emplace(*a.v.type_id, btmp); }
    } else if constexpr (isIntermediate<U>::value) {
      (*this)(b, a);
    } else {
      throw std::runtime_error("type check error");
    }
  }
};
template <class... Ts>
UnifyVisitor(Ts...) -> UnifyVisitor<Ts...>;

void TypeInferer::unify(IType::Value& a, IType::Value& b) {
  auto&& unifylambda = [&](IType::Value& a, IType::Value& b) -> void { unify(a, b); };
  UnifyVisitor vis{
      *this,
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
        unify(a.v.second, a.v.second);
      },
      [&](IType::Array& a, IType::Array& b) -> void {
        if (a.v.size != b.v.size) { throw std::runtime_error("array sizes are different"); }
        unify(a.v.v, b.v.v);
      },
      [&](IType::ListT& a, IType::ListT& b) -> void { unify(a.v, b.v); },
      [&](IType::Identified& a, IType::Identified& b) -> void {
        if (a.v.id != b.v.id) { throw std::runtime_error("identified types are different"); }
        unify(a.v.v, b.v.v);
      }};

  std::visit(vis, a.v, b.v);
}

bool TypeInferer::occurCheck(IType::Intermediate const& lv, IType::Value& rv) {
  auto id = *lv.v.type_id;
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
        if (*lv.v.type_id < *t.v.type_id) { *t.v.type_id = *lv.v.type_id; }
        return id == *t.v.type_id;
      },
      [&](IType::Alias& t) { return genmapper(t.v.v); },
      [&](auto&  /*t*/) { return false; },
  };
  return std::visit(vis, rv.v);
}

template <class F, class... Ts>
struct GeneralVisitor : F, Ts... {
  using V = IType::Value;
  using Ts::operator()...;
  template <class T>
  V operator()(T const& a) {
    if constexpr (is_boxed_t<T>::value) {
      return (*this)(a.getraw());
    } else if constexpr (std::is_empty_v<std::decay_t<T>>) {
      return V{a};
    }
    assert(!std::is_empty_v<std::decay_t<T>>);
    return V{IType::Unknown{}};
  }
  template <class T, int id>
  V operator()(CategoryWrapped<List, id, T> const& a) {
    return V{CategoryWrapped<List, id, T>{fmap(a.v, [&](V const& v) { return F::operator()(v); })}};
  }
  template <int id>
  V operator()(CategoryWrapped<List, id, IType::RecordKey> const& a) {
    return V{CategoryWrapped<List, id, IType::RecordKey>{fmap(a.v, [&](IType::RecordKey const& a) {
      return IType::RecordKey{a.key, F::operator()(a.v)};
    })}};
  }
  template <class T, int id, std::enable_if_t<!std::is_same_v<T, IType::IntermediateV>>>
  V operator()(CategoryWrapped<IdentCategory, id, T> const& a) {
    return V{CategoryWrapped<IdentCategory, id, T>{F::operator()(a.v)}};
  }
  V operator()(IType::Function const& a) {
    return V{IType::Function{std::pair(
        fmap(a.v.first, [&](V const& v) { return F::operator()(v); }), F::operator()(a.v.second))}};
  }
  V operator()(IType::Identified const& a) {
    return V{IType::Identified{a.v.id, F::operator()(a.v.v)}};
  }
  V operator()(IType::Alias const& a) { return V{IType::Alias{a.v.id, F::operator()(a.v.v)}}; }
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
                                     Map<int, IType::Value> const& typevar_map) {
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
        auto&& iter = typevar_map.find(*a.v.type_id);
        if (iter == typevar_map.cend()) {
          throw std::runtime_error("failed to infer variable for typeid " +
                                   std::to_string(*a.v.type_id));
        }
        return genmapper(iter->second);
      },
      [&](IType::TypeScheme const& a) {
        throw std::runtime_error("failed to instantiate type scheme for scheme-id" +
                                 std::to_string(*a.v.type_id));
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
    const std::shared_ptr<TypeEnvI> env, Map<int, IType::Value> const& typevar_map) {
  auto res = std::shared_ptr<TypeEnvH>();
  auto res_tmp = res;
  std::optional<std::shared_ptr<TypeEnvI>> e = env;
  while (e.has_value()) {
    for (auto&& [id, type] : env->map) { res_tmp->map.emplace(id, lowerIType(type, typevar_map)); }
    e = e.value()->child_env;
    if (e.has_value()) { res_tmp = res_tmp->expand(); }
  }
  return res;
}

IType::Intermediate TypeInferer::makeNewTypeVar(int level) {
  return IType::Intermediate{std::make_shared<int>(typevar_count++), level};
}
IType::TypeScheme TypeInferer::makeNewTypeScheme() {
  return IType::TypeScheme{std::make_shared<int>(typevar_count++), 0};
}

IType::Value TypeInferer::generalizeInternal(IType::Value const& t, int level,
                                             Map<int, int>& typevar_to_scheme) {
  auto&& get_type_scheme = [&](int tv_id) {
    if (typevar_to_scheme.count(tv_id) == 0) {
      auto newscheme = makeNewTypeScheme();
      typevar_to_scheme.emplace(tv_id, *newscheme.v.type_id);
    }
    return typevar_to_scheme.at(tv_id);
  };

  auto&& vis = GeneralVisitor{
      [&](IType::Value const& a) { return Box(generalizeInternal(a, level, typevar_to_scheme)); },
      [&](IType::TypeScheme const& a) { return IType::Value{a}; },
      [&](IType::Intermediate const& a) {
        if (a.v.level > level) {
          return IType::Value{
              IType::TypeScheme{std::make_shared<int>(get_type_scheme(*a.v.type_id)), 0}};
        }
        return IType::Value{a};
      }};
  return std::visit(vis, t.v);
}

IType::Value TypeInferer::generalize(IType::Value const& t, int level) {
  Map<int, int> typevar_to_scheme{};
  return generalizeInternal(t, level, typevar_to_scheme);
}

IType::Value TypeInferer::instantiateInternal(IType::Value const& t, int level,
                                              Map<int, int>& scheme_to_typevar) {
  auto&& get_type_var = [&](int scheme_id) {
    if (scheme_to_typevar.count(scheme_id) == 0) {
      auto newtv = makeNewTypeVar(level);
      scheme_to_typevar.emplace(scheme_id, *newtv.v.type_id);
    }
    return scheme_to_typevar.at(scheme_id);
  };
  auto&& vis = GeneralVisitor{
      [&](IType::Value const& a) { return Box(instantiateInternal(a, level, scheme_to_typevar)); },
      [&](IType::TypeScheme const& a) {
        return IType::Value{
            IType::Intermediate{std::make_shared<int>(get_type_var(*a.v.type_id)), level}};
      },
      [&](IType::Intermediate const& a) { return IType::Value{a}; }};
  return std::visit(vis, t.v);
}

IType::Value TypeInferer::instantiate(IType::Value const& t, int level) {
  Map<int, int> scheme_to_typevar{};
  return instantiateInternal(t, level, scheme_to_typevar);
}

IType::Value TypeInferer::inferInternal(LAst::expr& expr, std::shared_ptr<TypeEnvI> env,
                                        int level) {
  auto&& inferlambda = [&](LAst::expr& e) { return Box(inferInternal(e, env, level)); };
  decltype(env) env_v = env;

  assert(env_v != nullptr);
  auto&& vis = overloaded{
      [&](LAst::FloatLit&  /*a*/) -> IType::Value { return IType::Value{IType::Float{}}; },
      [&](LAst::IntLit&  /*a*/) -> IType::Value { return IType::Value{IType::Int{}}; },
      [&](LAst::BoolLit&  /*a*/) -> IType::Value { return IType::Value{IType::Bool{}}; },
      [&](LAst::StringLit&  /*a*/) -> IType::Value { return IType::Value{IType::String{}}; },
      [&](LAst::TupleLit& a) -> IType::Value {
        if (a.v.empty()) { return IType::Value{IType::Unit{}}; }
        return IType::Value{IType::Tuple{fmap(a.v, inferlambda)}};
      },
      [&](LAst::Symbol& a) -> IType::Value {
        auto t = env->search(a.v);
        if (!t.has_value()) {
          auto ext_iter = Intrinsic::ftable.find(a.v);
          if(ext_iter!=Intrinsic::ftable.cend()){
            return instantiate(ext_iter->second.mmmtype,level);
          }
          throw std::runtime_error("failed to look up symbol " + a.v);
        }
        return instantiate(t.value(), level);
      },

      [&](LAst::SelfLit&  /*a*/) {
        return this->self_t_holder;  // TODO
      },
      [&](LAst::TupleGet& a) -> IType::Value {
        auto ttype = inferlambda(a.v.expr).getraw();
        if (auto* tup = std::get_if<IType::Tuple>(&ttype.v)) {
          auto iter = std::next(tup->v.begin(), a.v.field);
          return *iter;
        }
        throw std::runtime_error("type check error in Tuple getter");
      },
      [&](LAst::StructLit& a) -> IType::Value {
        return IType::Value{IType::Record{fmap(a.v, [&](LAst::StructKey& e) {
          return IType::RecordKey{e.key, inferlambda(e.v)};
        })}};
      },
      [&](LAst::StructGet& a) -> IType::Value {
        auto ttype = inferlambda(a.v.expr).getraw();
        if (auto* rec = std::get_if<IType::Record>(&ttype.v)) {
          auto iter = std::find_if(rec->v.begin(), rec->v.end(),
                                   [&](IType::RecordKey const& k) { return k.key == a.v.field; });
          if (iter != rec->v.end()) { return iter->v; }
        }
        throw std::runtime_error("type check error in Record Type getter");
      },
      [&](LAst::ArrayLit& a) -> IType::Value {
        List<Box<IType::Value>> arglist = fmap(a.v, inferlambda);
        foldl(arglist, [&](IType::Value& a, IType::Value& b) {
          this->unify(a, b);
          return b;
        });
        return IType::Value{IType::Array{*arglist.begin(), static_cast<int>(arglist.size())}};
      },
      [&](LAst::ArrayGet& a) -> IType::Value {
        auto ttype = inferlambda(a.v.expr).getraw();
        if (auto* rec = std::get_if<IType::Array>(&ttype.v)) { return rec->v.v; }
        throw std::runtime_error("type check error in Array Type getter");
      },
      [&](LAst::ArraySize& a) -> IType::Value {
        inferlambda(a.v);
        return IType::Value{IType::Int{}};
      },
      [&](LAst::Lambda& a) -> IType::Value {
        auto newenv = env_v->expand();
        auto atype = fmap<List>(a.v.args, [&](LAst::Id& a) -> Box<IType::Value> {
          return Box(a.type.value_or(IType::Value{makeNewTypeVar(level)}));
        });
        auto atype_iter = atype.cbegin();
        for(auto& a: a.v.args){
          newenv->addToMap(a.v,*atype_iter);
          atype_iter++;
        }
        auto body_t = inferInternal(a.v.body, newenv, level+1);
        auto body_t_generic = generalize(body_t, level);
        this->self_t_holder = body_t_generic;
        return IType::Value{IType::Function{std::pair(atype, body_t_generic)}};
      },
      [&](LAst::Sequence& a) -> IType::Value {
        auto unit = IType::Value{IType::Unit{}};
        auto first_t = inferlambda(a.v.first);
        this->unify(first_t, unit);
        return inferlambda(a.v.second);
      },
      [&](LAst::NoOp&  /*a*/) -> IType::Value { return IType::Value{IType::Unit{}}; },
      [&](LAst::Let& a) -> IType::Value {
        auto newenv = env_v->expand();
        if (!a.v.id.type.has_value()) { a.v.id.type = IType::Value{makeNewTypeVar(level)}; }
        newenv->addToMap(a.v.id.v, a.v.id.type.value());
        auto exprtype = inferInternal(a.v.expr, env, level + 1);
        this->unify(a.v.id.type.value(), exprtype);
        auto rvaluet = inferInternal(a.v.body, newenv, level);
        auto rvaluet_generic = generalize(rvaluet, level);
        return rvaluet_generic;
      },
      [&](LAst::LetTuple& a) -> IType::Value {
        auto newenv = env_v->expand();
        for (auto&& arg : a.v.id) {
          if (!arg.type) { arg.type = IType::Value{makeNewTypeVar(level)}; }
          newenv->addToMap(arg.v, arg.type.value());
        }
        auto exprtype = inferInternal(a.v.expr, env, level + 1);
        if (auto* tup = std::get_if<IType::Tuple>(&exprtype.v)) {
          auto&& iter = tup->v.begin();
          for (auto&& arg : a.v.id) {
            this->unify(arg.type.value(), *iter);
            std::advance(iter, 1);
          }
          auto rvaluet = inferInternal(a.v.body, newenv, level);
          auto rvaluet_generic = generalize(rvaluet, level);
          return rvaluet_generic;
        }
        throw std::runtime_error("type check error in LetTuple");
      },
      [&](LAst::App& a) -> IType::Value {
        auto ftype = inferlambda(a.v.callee).getraw();
        if (auto* fun = std::get_if<IType::Function>(&ftype.v)) {
          auto args = fmap(a.v.args, inferlambda);
          zipWith(fun->v.first, args, [&](auto& a, auto& b) { this->unify(a, b); });
          return fun->v.second;
        }
        throw std::runtime_error("type check error in App");
      },
      [&](LAst::If& a) -> IType::Value {
        auto condtype = inferlambda(a.v.cond);
        auto ftype = IType::Value{IType::Float{}};
        this->unify(condtype, ftype);
        IType::Value thentype = inferlambda(a.v.vthen);
        if (a.v.velse.has_value()) {
          auto elsetype = inferlambda(a.v.velse.value());
          this->unify(thentype, elsetype);
          return thentype;
        }
        auto voidt = IType::Value{IType::Unit{}};
        this->unify(thentype, voidt);
        return voidt;
      }};
  return std::visit(vis, expr.v);
}

}  // namespace mimium
