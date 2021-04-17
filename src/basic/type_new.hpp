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
  using Aggregate = CategoryWrapped<C, ID, Value>;
  // T1 x T2 x T3 ...
  using Tuple = Aggregate<List>;
  // T1 | T2 | T3...
  using Variant = Aggregate<List, 1>;
  // T1 -> T2
  template <class T>
  using FunCategory = Pair<List<T>, T>;
  using Function = Aggregate<FunCategory>;

  template <class T>
  struct ArrayCategory {
    T v;
    int size;
  };
  // [T]
  using Array = Aggregate<ArrayCategory>;
  // [[T]]
  using ListT = CategoryWrapped<IdentCategory, 1, Value>;
  //{key:T1,key:T2,...}
  template <class Identifier>
  struct RecordCategory {
    Identifier key;
    Value v;
  };
  using Record = CategoryWrapped<List, 0, RecordCategory<std::string>>;

  // my_t of int
  // mostly used for variants.
  // type color = Green of int| Red of Int |....
  template <class ID>
  struct IdentifiedCategory {
    ID id;
    Value v;
  };
  using Identified = CategoryWrapped<IdentifiedCategory, 0, int>;
  using Alias = CategoryWrapped<IdentifiedCategory, 0, std::string>;

  struct Unknown {};
  struct Intermediate {
    std::shared_ptr<int> type_id;
  };

  // Types for MIR~LIR level expression, which contains Pointer, without variant, list and named
  // newtype.
  using Pointer = CategoryWrapped<IdentCategory, 2, Value>;

  // Generic Types TBD
  template <class Id>
  using TypeScheme = typename CategoryWrapped<IdentifiedCategory, 2, Id>::type;

  inline static SExpr toSExpr(Value const& v) { return std::visit(to_sexpr_visitor, v.getraw().v); }
  inline const static auto listvisithelper = [](const auto& t) {
    return foldl(fmap(t, [](Value const& a) { return toSExpr(a); }),
                 [](auto&& a, auto&& b) { return cons(a, b); });
  };
  inline const static auto recordvisithelper = [](const auto& t) {
    return foldl(fmap(t,
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
      [](Tuple const& t) { return cons(makeSExpr("tuple"), listvisithelper(t.v)); },
      [](Record const& t) { return cons(makeSExpr("record"), recordvisithelper(t.v)); },
      [](Variant const& t) { return cons(makeSExpr("variant"), listvisithelper(t.v)); },
      [](Function const& t) {
        return cons(makeSExpr("function"), cons(listvisithelper(t.v.first), toSExpr(t.v.second)));
      },
      [](Array const& t) { return cons(makeSExpr("array"), toSExpr(t.v.v)); },
      [](ListT const& t) { return cons(makeSExpr("list"), toSExpr(t.v)); },
      [](Pointer const& t) { return cons(makeSExpr("pointer"), toSExpr(t.v)); },
      [](Intermediate const& t) {
        return makeSExpr({"TypeVar", std::to_string(*t.type_id)});
      },
      [](Identified const& t) { return cons(makeSExpr("newtype"), toSExpr(t.v.v)); },
      [](Alias const& t) { return cons(makeSExpr("alias"), toSExpr(t.v.v)); },
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
  using RecordKey = baset::RecordCategory<std::string>;

  using ListT = baset::ListT;
  using Identified = baset::Identified;
  using Alias = baset::Alias;

  using Intermediate = baset::Intermediate;
  using Unknown = baset::Unknown;
  using type = std::variant<Unit, Bool, Int, Float, String, Variant, Tuple, Function, Array, Record,
                            Identified, ListT, Intermediate, Alias, Unknown>;
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
  using Alias = baset::Alias;

  // Intermediate, Unknown type are removed.
  using type = std::variant<Unit, Bool, Int, Float, String, Variant, Tuple, Function, Array, Record,
                            Identified, Alias, ListT>;
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

inline auto makeUnknownAlias(std::string const& name) {
  auto itype = IType::Value{IType::Unknown{}};
  return IType::Value{IType::Alias{name, itype}};
}

}  // namespace mimium