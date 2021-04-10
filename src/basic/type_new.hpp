#pragma once
#include "abstractions.hpp"
#include "sexpr.hpp"
namespace mimium {

template <class Value>
struct Type {
  struct Unit {};
  struct Bool {};
  struct Int {};
  struct Float {};
  struct String {};
  template <template <class...> class C, int ID = 0>
  using Aggregate = CategoryWrapped<C, Value, ID>;
  // T1 x T2 x T3 ...
  using Tuple = Aggregate<List>;
  // T1 | T2 | T3...
  using Variant = Aggregate<List, 1>;
  // T1 -> T2
  using Function = Aggregate<Pair>;

  template <class T>
  struct ArrayCategory {
    T v;
    int size;
  };
  // [T]
  using Array = Aggregate<ArrayCategory>;
  // [[T]]
  using ListT = CategoryWrapped<IdentCategory, Value, 1>;
  //{key:T1,key:T2,...}
  template <class Identifier>
  struct RecordCategory {
    Identifier key;
    Value v;
  };
  using Record = CategoryWrapped<List, RecordCategory<std::string>>;

  // my_t of int
  // mostly used for variants.
  // type color = Green of int| Red of Int |....
  template <class ID>
  struct IdentifiedCategory {
    ID id;
    Value v;
  };
  using Identified = CategoryWrapped<IdentifiedCategory, int>;

  struct Unknown {};
  struct Intermediate {
    int type_id;
  };

  // Types for MIR~LIR level expression, which contains Pointer, without variant, list and named
  // newtype.
  using Pointer = CategoryWrapped<IdentCategory, Value, 2>;

  // Generic Types TBD
  template <class Id>
  using TypeScheme = typename CategoryWrapped<IdentifiedCategory, Id, 2>::type;

  inline static SExpr toSExpr(Value const& v) { return std::visit(to_sexpr_visitor, v.getraw().v); }
  inline const static auto listvisithelper = [](const auto& t) {
    return foldl(fmap(t.v, [](Value const& a) { return toSExpr(a); }),
                 [](auto&& a, auto&& b) { return cons(a, b); });
  };
  inline const static auto recordvisithelper = [](const auto& t) {
    return foldl(fmap(t.v,
                      [](RecordCategory<std::string> const& a) {
                        return cons(makeSExpr(a.key), toSExpr(a.v));
                      }),
                 [](auto&& a, auto&& b) { return cons(a, b); });
  };
  inline const static auto to_sexpr_visitor = overloaded{
      [](Unit const& /**/) { return makeSExpr("unit"); },
      [](Float const& /**/) { return makeSExpr("float"); },
      [](Int const& /**/) { return makeSExpr("int"); },
      [](Bool const& /**/) { return makeSExpr("bool"); },
      [](String const& /**/) { return makeSExpr("String"); },
      [](Unknown const& /**/) { return makeSExpr("Unknown"); },
      [](Tuple const& t) { return cons(makeSExpr("tuple"), listvisithelper(t)); },
      [](Record const& t) { return cons(makeSExpr("record"), recordvisithelper(t)); },
      [](Variant const& t) { return cons(makeSExpr("variant"), listvisithelper(t)); },
      [](Function const& t) {
        return cons(makeSExpr("function"), cons(toSExpr(t.v.first), toSExpr(t.v.second)));
      },
      [](Array const& t) { return cons(makeSExpr("array"), toSExpr(t.v.v)); },
      [](ListT const& t) { return cons(makeSExpr("list"), toSExpr(t.v)); },
      [](Pointer const& t) { return cons(makeSExpr("pointer"), toSExpr(t.v)); },
      [](Intermediate const& t) {
        return makeSExpr({"TypeVar", std::to_string(t.type_id)});
      },
      [](Identified const& t) { return cons(makeSExpr("newtype"),toSExpr(t.v.v)); },
      [](auto const& a) { return makeSExpr(""); }};
};
struct IType {
  struct Value;
  using baset = Type<Box<Value>>;
  using Unit = baset::Unit;
  using Bool = baset::Bool;
  using Int = baset::Int;
  using Float = baset::Float;
  using String = baset::String;
  using Variant = baset::Variant;
  using Tuple = baset::Tuple;
  using Function = baset::Function;
  using Array = baset::Array;
  using Record = baset::Record;
  using ListT = baset::ListT;
  using Identified = baset::Identified;
  using Intermediate = baset::Intermediate;
  using Unknown = baset::Unknown;
  using type = std::variant<Unit, Bool, Int, Float, String, Variant, Tuple, Function, Array, Record,
                            Identified, ListT, Intermediate, Unknown>;
  struct Value {
    using baset = Type<Box<Value>>;
    type v;
    operator type&() { return v; }
    operator const type&() const { return v; }
  };
};

struct HType {
  struct Value;
  using baset = Type<Box<Value>>;
  using Unit = baset::Unit;
  using Bool = baset::Bool;
  using Int = baset::Int;
  using Float = baset::Float;
  using String = baset::String;
  using Variant = baset::Variant;
  using Tuple = baset::Tuple;
  using Function = baset::Function;
  using Array = baset::Array;
  using Record = baset::Record;
  using ListT = baset::ListT;
  using Identified = baset::Identified;
  // Intermediate, Unknown type are removed.
  using type = std::variant<Unit, Bool, Int, Float, String, Variant, Tuple, Function, Array, Record,
                            Identified, ListT>;
  struct Value {
    using baset = Type<Box<Value>>;
    type v;

    operator type&() { return v; }
    operator const type&() const { return v; }
  };
};

struct LType {
  struct Value;
  using baset = Type<Box<Value>>;
  using Unit = baset::Unit;
  using Bool = baset::Bool;
  using Int = baset::Int;
  using Float = baset::Float;
  using String = baset::String;
  using Tuple = baset::Tuple;
  using Function = baset::Function;
  using Array = baset::Array;
  using Identified = baset::Identified;

  using Pointer = baset::Pointer;
  // LType does not have Record, List, variant and intermediate typevars.
  // And, Pointer type is introduced.
  using type =
      std::variant<Unit, Bool, Int, Float, String, Tuple, Function, Array, Identified, Pointer>;
  struct Value {
    using baset = Type<Box<Value>>;

    type v;
    operator type&() { return v; }
    operator const type&() const { return v; }
  };
};

template <class T>
SExpr toString(typename T::Value const& t) {
  return toString(toSExpr(t));
}

static_assert(std::is_copy_constructible_v<IType::Value>);

template <class ID, class Value>
struct Alias {
  using idtype = ID;
  using map = Map<ID, Value>;
  idtype id;
  using Ttype = std::variant<Value, Box<Alias<ID, Value>>>;
  Ttype a;
  inline static SExpr toSExprA(Ttype const& t) {
    return std::visit(overloaded{[](const Alias& a) { return toSExpr(a); },
                                 [](const Value& a) { return Value::baset::toSExpr(a); }},
                      t);
  }
  inline static SExpr toSExpr(Alias const& t) { return cons(makeSExpr(t.id), toSExprA(t.a)); }
};

using Alias_t = Alias<std::string, IType::Value>;
using ITypeOrAlias = Alias_t::Ttype;

inline SExpr toSExpr(ITypeOrAlias const& t) { return Alias_t::toSExprA(t); }

inline auto toString(ITypeOrAlias const& t) { return toString(toSExpr(t)); }

inline auto makeUnknownAlias(std::string const& name) {
  auto itype = IType::Value{IType::Unknown{}};
  return ITypeOrAlias{Alias_t{name, itype}};
}
struct TypeResolver {
  // TypeIdの集合と束縛のリスト
  template <class T>
  using Binding = Pair<T, T>;

  using Constraints = Set<Binding<IType>>;
  using map = Map<IType, HType>;
};

// 変数と型の写像がスコープごとに
template <class Key, class T>
struct Environment {
  struct Value {
    Map<Key, Pair<T, Box<Environment>>> map;
  };
};
template <class Expr, class T>
using TypeEnv = Environment<Expr, T>;

template <class AST>
struct TypeInferer {
  template <class T>
  using TypeEnv = TypeEnv<AST, T>;

  using InputType = Either<IType, Alias<std::string, IType>>;
  using InputAliasMap = Alias<std::string, IType>::map;
  // ASTを入れると中間変数を含む型環境とエイリアスの写像が帰ってくる
  using Pass1 = Pair<TypeEnv<InputType>, InputAliasMap>(AST);

  // エイリアスを全部剥がした型環境を返す
  using Pass2 = TypeEnv<IType>(TypeEnv<InputType>, InputAliasMap);
  //中間変数を含む型環境を受け取り、変数同士の制約を作る
  using Pass3 = TypeResolver::Constraints(TypeEnv<IType>);
  //変数の制約を受け取り、中間変数を含む型集合->中間変数を取り除いた型集合 の写像をつくる
  using Pass4 = TypeResolver::map(TypeResolver::Constraints);
  //中間変数を含む型環境と、中間変数を含む型集合->中間変数を取り除いた型集合の写像を受け取り、
  //中間変数を取り除いた型環境を返す
  using Pass5 = TypeEnv<HType>(TypeEnv<InputType>, TypeResolver::map);

  //入力の alias|Itype -> alias|Htype にマップし直す
  using OutputAlias = Alias<std::string, HType>;
  using OutputType = Either<HType, OutputAlias>;
  using OutputAliasMap = OutputAlias::map;
  using Pass6 = Pair<TypeEnv<OutputType>, OutputAliasMap>(TypeEnv<HType>, InputAliasMap);
  // 全体Ïとしては、ASTを入れると中間変数がない(型|Alias)の環境が帰ってくる
  using type = Pair<TypeEnv<OutputType>, OutputAliasMap>(AST);
};
struct Test {};
TypeInferer<Test> inferer;

}  // namespace mimium