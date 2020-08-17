/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "basic/helper_functions.hpp"

namespace mimium {
// https://medium.com/@dennis.luxen/breaking-circular-dependencies-in-recursive-union-types-with-c-17-the-curious-case-of-4ab00cfda10d
namespace types {
enum class Kind { VOID, PRIMITIVE, POINTER, AGGREGATE, INTERMEDIATE };
}

namespace types {

template <class T>
constexpr bool is_recursive_type = T::is_aggregatetype;

struct PrimitiveType {
  PrimitiveType() = default;
  constexpr inline static Kind kind = Kind::PRIMITIVE;
};

inline bool operator==(const PrimitiveType& t1, const PrimitiveType& t2) {
  return true;
};
inline bool operator!=(const PrimitiveType& t1, const PrimitiveType& t2) {
  return false;
};

struct None : PrimitiveType {};
struct Void : PrimitiveType {};
struct Float : PrimitiveType {};
struct String : PrimitiveType {};

// Intermediate Type for type inference.

struct TypeVar;

struct Ref;
struct Pointer;
struct Function;
struct Closure;
struct Array;
struct Struct;
struct Tuple;
// struct Time;

struct Alias;

using rRef = Rec_Wrap<Ref>;
using rPointer = Rec_Wrap<Pointer>;
using rTypeVar = Rec_Wrap<TypeVar>;
using rFunction = Rec_Wrap<Function>;
using rClosure = Rec_Wrap<Closure>;
using rArray = Rec_Wrap<Array>;
using rStruct = Rec_Wrap<Struct>;
using rTuple = Rec_Wrap<Tuple>;
using rArray = Rec_Wrap<Array>;
using rAlias = Rec_Wrap<Alias>;

using Value =
    std::variant<None, Void, Float, String, rRef, rTypeVar, rPointer, rFunction,
                 rClosure, rArray, rStruct, rTuple, rAlias>;

struct ToStringVisitor;

struct TypeVar : std::enable_shared_from_this<TypeVar> {
  explicit TypeVar(int i) : index(i) {}
  int index;
  Value contained = types::None();
  std::optional<std::shared_ptr<TypeVar>> prev = std::nullopt;
  std::optional<std::shared_ptr<TypeVar>> next = std::nullopt;
  template <bool IS_PREV>
  std::shared_ptr<TypeVar> getLink() {
    std::optional<std::shared_ptr<TypeVar>> tmp = shared_from_this();
    if constexpr (IS_PREV) {
      while (tmp.value()->prev.has_value()) {
        tmp = tmp.value()->prev;
      }
    } else {
      while (tmp.value()->next.has_value()) {
        tmp = tmp.value()->next;
      }
    }
    return tmp.value();
  }
  auto getFirstLink() { return getLink<true>(); }
  auto getLastLink() { return getLink<false>(); }

  int getIndex() { return index; }
  void setIndex(int newindex) { index = newindex; }
  constexpr inline static Kind kind = Kind::INTERMEDIATE;
};

inline bool operator==(const TypeVar& t1, const TypeVar& t2) {
  return t1.index == t2.index;
}
inline bool operator!=(const TypeVar& t1, const TypeVar& t2) {
  return t1.index != t2.index;
}

struct PointerBase {
  constexpr inline static Kind kind = Kind::POINTER;
};
struct Ref : PointerBase {
  Ref() = default;
  explicit Ref(Value v) : val(std::move(v)){};
  Value val;
};

inline bool operator==(const Ref& t1, const Ref& t2) {
  return t1.val == t2.val;
}
inline bool operator!=(const Ref& t1, const Ref& t2) {
  return t1.val != t2.val;
}
struct Pointer : PointerBase {
  Pointer() = default;
  explicit Pointer(Value v) : val(std::move(v)){};
  Value val;
};
inline bool operator==(const Pointer& t1, const Pointer& t2) {
  return t1.val == t2.val;
}
inline bool operator!=(const Pointer& t1, const Pointer& t2) {
  return t1.val != t2.val;
}
struct Aggregate {
  constexpr inline static Kind kind = Kind::AGGREGATE;
};
// struct Time : Aggregate {
//   Time()=default;
//   explicit Time(Value v):val(std::move(v)){};
//   Value val;
//   Float time;
// };
// inline bool operator==(const Time& t1, const Time& t2) {
//   return t1.val == t2.val;
// }
// inline bool operator!=(const Time& t1, const Time& t2) {
//   return t1.val != t2.val;
// }
struct Function : Aggregate {
  Function() = default;
  explicit Function(Value ret_type_p, std::vector<Value> arg_types_p)
      : arg_types(std::move(arg_types_p)), ret_type(std::move(ret_type_p)){};
  // [[maybe_unused]]void init(std::vector<Value> arg_types_p, Value ret_type_p)
  // {
  //   arg_types = std::move(arg_types_p);
  //   ret_type = std::move(ret_type_p);
  //   ret_type.emplace()
  // }
  Value ret_type;
  std::vector<Value> arg_types;

  Value& getReturnType() { return ret_type; }
  std::vector<Value>& getArgTypes() { return arg_types; }

  [[maybe_unused]] static Function create(
      const types::Value& ret, const std::vector<types::Value>& arg) {
    return Function(ret, arg);
  }
  template <class... Args>
  static std::vector<Value> createArgs(Args&&... args) {
    std::vector<Value> a = {std::forward<Args>(args)...};
    return a;
  }
};

inline bool operator==(const Function& t1, const Function& t2) {
  return t1.ret_type == t2.ret_type && t1.arg_types == t2.arg_types;
}
inline bool operator!=(const Function& t1, const Function& t2) {
  return !(t1 == t2);
}

struct Closure : Aggregate {
  Ref fun;
  Value captures;
  explicit Closure(Ref fun, Value captures)
      : fun(std::move(fun)), captures(std::move(captures)){};
  explicit Closure(Value fun, Value captures)
      : fun(std::move(rv::get<Ref>(fun))), captures(std::move(captures)){};
  Closure() = default;
};
inline bool operator==(const Closure& t1, const Closure& t2) {
  return t1.fun == t2.fun && t1.captures == t2.captures;
}
inline bool operator!=(const Closure& t1, const Closure& t2) {
  return !(t1 == t2);
}
struct Array : Aggregate {
  Value elem_type;
  int size;
  explicit Array(Value elem, int size = 0)
      : elem_type(std::move(elem)), size(size) {}
};
inline bool operator==(const Array& t1, const Array& t2) {
  return (t1.elem_type == t2.elem_type) && (t1.size == t2.size);
}
inline bool operator!=(const Array& t1, const Array& t2) { return !(t1 == t2); }
struct Tuple : Aggregate {
  std::vector<Value> arg_types;
  explicit Tuple(std::vector<Value> types) : arg_types(std::move(types)) {}
};

inline bool operator==(const Tuple& t1, const Tuple& t2) {
  return (t1.arg_types == t2.arg_types);
};
inline bool operator!=(const Tuple& t1, const Tuple& t2) { return !(t1 == t2); }
struct Struct : Aggregate {
  struct Keytype {
    std::string field;
    Value val;
  };
  std::vector<Keytype> arg_types;
  explicit Struct(std::vector<Keytype> types) : arg_types(std::move(types)) {}
  explicit operator Tuple() {
    std::vector<Value> res;
    std::for_each(arg_types.begin(), arg_types.end(),
                  [&](const auto& c) { res.push_back(c.val); });
    return Tuple(res);
  }
  explicit operator Tuple() const {  // cast back to tuple
    std::vector<Value> res;
    std::for_each(arg_types.begin(), arg_types.end(),
                  [&](const auto& c) { res.push_back(c.val); });
    return Tuple(res);
  }
};

inline bool operator==(const Struct::Keytype& t1, const Struct::Keytype& t2) {
  return (t1.field == t2.field) && (t1.val == t2.val);
};
inline bool operator!=(const Struct::Keytype& t1, const Struct::Keytype& t2) {
  return !(t1 == t2);
}
inline bool operator==(const Struct& t1, const Struct& t2) {
  return (t1.arg_types == t2.arg_types);
};
inline bool operator!=(const Struct& t1, const Struct& t2) {
  return !(t1 == t2);
}

struct Alias : Aggregate {
  std::string name;
  Value target;
  explicit Alias(std::string name, Value target)
      : name(std::move(name)), target(std::move(target)) {}
};
inline bool operator==(const Alias& t1, const Alias& t2) {
  return (t1.name == t2.name);
};
inline bool operator!=(const Alias& t1, const Alias& t2) { return !(t1 == t2); }
bool isTypeVar(types::Value t);

struct ToStringVisitor {
  bool verbose = false;
  [[nodiscard]] std::string join(const std::vector<types::Value>& vec,
                                 std::string delim) const {
    std::string res;
    if (!vec.empty()) {
      res = std::accumulate(
          std::next(vec.begin()), vec.end(), std::visit(*this, *vec.begin()),
          [&](std::string a, const Value& b) {
            return std::move(a) + delim + std::visit(*this, b);
          });
    }
    return res;
  }
  std::string operator()(None) const { return "none"; }
  std::string operator()(const TypeVar& v) const {
    return "TypeVar" + std::to_string(v.index);
  }
  std::string operator()(Void) const { return "void"; }
  std::string operator()(Float) const { return "float"; }
  std::string operator()(String) const { return "string"; }
  std::string operator()(const Ref& r) const {
    return std::visit(*this, r.val) + "&";
  }
  std::string operator()(const Pointer& r) const {
    return std::visit(*this, r.val) + "*";
  }
  std::string operator()(const Function& f) const {
    return "(" + join(f.arg_types, ",") + ") -> " +
           std::visit(*this, f.ret_type);
  }
  std::string operator()(const Closure& c) const {
    return "cls{ " + (*this)(c.fun) + " , " + std::visit(*this, c.captures) +
           " }";
  }
  std::string operator()(const Array& a) const {
    return "[" + std::visit(*this, a.elem_type) + "x" + std::to_string(a.size) +
           "]";
  }
  std::string operator()(const Struct& s) const {
    std::string str = "{";
    for (auto& arg : s.arg_types) {
      str += arg.field + ":" + std::visit(*this, arg.val) + ",";
    }
    return str.substr(0, str.size() - 1) + "}";
  }
  std::string operator()(const Tuple& t) const {
    return "(" + join(t.arg_types, ",") + ")";
  }
  // std::string operator()(const Time& t) const {
  //   return std::visit(*this, t.val) + "@";
  // }
  std::string operator()(const Alias& a) const {
    return a.name + ((verbose) ? ": " + std::visit(*this, a.target) : "");
  }
};

Value getFunRettype(types::Value& v);
std::optional<Value> getNamedClosure(types::Value& v);

static ToStringVisitor tostrvisitor;
std::string toString(const Value& v, bool verbose = false);
void dump(const Value& v, bool verbose = false);

struct KindVisitor {
  template <class T>
  Kind operator()(T t) {
    return T::kind;
  }
  template <class T>
  Kind operator()(Rec_Wrap<T> t) {
    return T::kind;
  }
};
static KindVisitor kindvisitor;

Kind kindOf(const Value& v);
bool isPrimitive(const Value& v);
}  // namespace types

class TypeEnv {
 private:
  int64_t typeid_count{};

 public:
  TypeEnv() : env() {}
  std::unordered_map<std::string, types::Value> env;
  std::deque<std::shared_ptr<types::TypeVar>> tv_container;
  std::shared_ptr<types::TypeVar> createNewTypeVar() {
    auto& ref = tv_container.emplace_back(
        std::make_shared<types::TypeVar>(typeid_count++));
    return ref;
  }
  std::shared_ptr<types::TypeVar> findTypeVar(int tindex) {
    auto iter =
        std::find_if(tv_container.begin(), tv_container.end(),
                     [&](auto tvptr) { return tindex == tvptr->index; });
    if (iter == tv_container.end()) {
      throw std::runtime_error("failed to find typevar for specified index");
    }
    return *iter;
  }
  bool exist(std::string key) { return (env.count(key) > 0); }
  auto begin() { return env.begin(); }
  auto end() { return env.end(); }
  types::Value* tryFind(std::string key) {
    auto it = env.find(key);
    types::Value* res;
    res = (it == env.end()) ? nullptr : &it->second;
    return res;
  }
  types::Value& find(std::string key) {
    auto res = tryFind(key);
    if (res == nullptr) {
      throw std::logic_error("Could not find type for variable \"" + key +
                             "\"");
    }
    return *res;
  }

  auto emplace(std::string key, types::Value typevar) {
    return env.insert_or_assign(key, typevar);
  }
  void replaceTypeVars();

  std::string toString(bool verbose = false);
  void dump(bool verbose = false);
  void dumpTvLinks();
};

}  // namespace mimium