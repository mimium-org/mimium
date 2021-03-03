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

namespace types {
enum class Kind { VOID, PRIMITIVE, POINTER, AGGREGATE, INTERMEDIATE };
}

namespace types {

struct PrimitiveType {
  PrimitiveType() = default;
};

inline bool operator==(const PrimitiveType& /*t1*/, const PrimitiveType& /*t2*/) { return true; }

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

using rRef = Box<Ref>;
using rPointer = Box<Pointer>;
using rTypeVar = Box<TypeVar>;
using rFunction = Box<Function>;
using rClosure = Box<Closure>;
using rArray = Box<Array>;
using rStruct = Box<Struct>;
using rTuple = Box<Tuple>;
using rArray = Box<Array>;
using rAlias = Box<Alias>;

using Value = std::variant<None, Void, Float, String, rRef, rTypeVar, rPointer, rFunction, rClosure,
                           rArray, rStruct, rTuple, rAlias>;

struct ToStringVisitor;

inline bool operator==(const Ref& t1, const Ref& t2);
inline bool operator==(const Pointer& t1, const Pointer& t2);
inline bool operator==(const TypeVar& t1, const TypeVar& t2);
inline bool operator==(const Alias& t1, const Alias& t2);
inline bool operator==(const Closure& t1, const Closure& t2);
inline bool operator==(const Function& t1, const Function& t2);
inline bool operator==(const Struct& t1, const Struct& t2);
inline bool operator==(const Array& t1, const Array& t2);
inline bool operator==(const Tuple& t1, const Tuple& t2);

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
      while (tmp.value()->prev.has_value()) { tmp = tmp.value()->prev; }
    } else {
      while (tmp.value()->next.has_value()) { tmp = tmp.value()->next; }
    }
    return tmp.value();
  }
  auto getFirstLink() { return getLink<true>(); }
  auto getLastLink() { return getLink<false>(); }

  int getIndex() const { return index; }
  void setIndex(int newindex) { index = newindex; }
};

inline bool operator==(const TypeVar& t1, const TypeVar& t2) { return t1.index == t2.index; }

struct Ref {
  Value val;
};

inline bool operator==(const Ref& t1, const Ref& t2) { return t1.val == t2.val; }

struct Pointer {
  Value val;
};
inline bool operator==(const Pointer& t1, const Pointer& t2) { return t1.val == t2.val; }
// Helper function to make pointer to pointer type
// Because nested aggregate initialization like Pointer{Pointer{Float}} interpreted as copy
// construction.
inline auto makePointer(types::Value&& t) {
  types::Pointer res;
  res.val = std::forward<types::Value>(t);
  return std::forward<types::Value>(res);
}
inline auto makePointer(types::Value const& t) {
  types::Pointer res;
  res.val = t;
  return res;
}

struct Function {
  Value ret_type;
  std::vector<Value> arg_types;
};

inline bool operator==(const Function& t1, const Function& t2) {
  bool ret_same = t1.ret_type == t2.ret_type;
  bool arg_same = t1.arg_types == t2.arg_types;
  return ret_same && arg_same;
}

struct Closure {
  Ref fun;
  Value captures;
};
inline bool operator==(const Closure& t1, const Closure& t2) {
  return t1.fun == t2.fun && t1.captures == t2.captures;
}

struct Array {
  Value elem_type;
  int size;
};
inline bool operator==(const Array& t1, const Array& t2) {
  return (t1.elem_type == t2.elem_type) && (t1.size == t2.size);
}

inline bool isArraySizeVariable(const Array& arr) { return arr.size == 0; }

struct Tuple {
  std::vector<Value> arg_types;
};

inline bool operator==(const Tuple& t1, const Tuple& t2) { return (t1.arg_types == t2.arg_types); }

struct Struct {
  struct Keytype {
    std::string field;
    Value val;
  };
  std::vector<Keytype> arg_types;
};

// return 0-based index and its type of Struct type fields for the key string.

inline std::pair<int, types::Value> getField(types::Struct const& sttype, std::string const& key) {
  auto iter = std::find_if(sttype.arg_types.cbegin(), sttype.arg_types.cend(),
                           [&](const auto& a) { return a.field == key; });
  if (iter == sttype.arg_types.cend()) {
    throw std::logic_error("failed to find \"" + key + "\" field for struct type");
  }
  auto index = std::distance(sttype.arg_types.cbegin(), iter);
  return std::pair(index, sttype.arg_types.at(index).val);
};

inline bool operator==(const Struct::Keytype& t1, const Struct::Keytype& t2) {
  return (t1.field == t2.field) && (t1.val == t2.val);
}

inline bool operator==(const Struct& t1, const Struct& t2) {
  return (t1.arg_types == t2.arg_types);
}

struct Alias {
  std::string name;
  Value target;
};
inline bool operator==(const Alias& t1, const Alias& t2) { return (t1.name == t2.name); }
bool isTypeVar(types::Value t);

template <class T>
inline constexpr bool is_pointer_t = std::is_same_v<T, Pointer> || std::is_same_v<T, Ref>;

inline bool isPointer(types::Value t) {
  return std::holds_alternative<Box<Pointer>>(t) || std::holds_alternative<Box<Ref>>(t);
}

// Note(tomoya):this weird sfinae need to prevent from a compile error on Xcode
// clang that confuses candidates with iterator's comparison operator
template <typename T, typename U,
          typename std::enable_if_t<
              !std::is_same_v<std::decay_t<std::move_iterator<U>>, std::decay_t<T>>>>
bool operator!=(T t1, T t2) {
  return !(t1 == t2);
}

constexpr size_t fixed_delaysize = 44100;
inline auto getDelayStruct() {
  return types::Alias{"MmmRingBuf", types::Tuple{{types::Float{}, types::Float{},
                                                  types::Array{types::Float{}, fixed_delaysize}}}};
}

struct ToStringVisitor {
  bool verbose = false;
  [[nodiscard]] std::string join(const std::vector<types::Value>& vec,
                                 std::string const& delim) const {
    std::string res;
    if (!vec.empty()) {
      res = std::accumulate(std::next(vec.begin()), vec.end(), std::visit(*this, *vec.begin()),
                            [&](std::string a, const Value& b) {
                              return std::move(a) + delim + std::visit(*this, b);
                            });
    }
    return res;
  }
  std::string operator()(None /*unused*/) const { return "none"; }
  std::string operator()(const TypeVar& v) const { return "TypeVar" + std::to_string(v.index); }
  std::string operator()(Void /*unused*/) const { return "void"; }
  std::string operator()(Float /*unused*/) const { return "float"; }
  std::string operator()(String /*unused*/) const { return "string"; }
  std::string operator()(const Ref& r) const { return std::visit(*this, r.val) + "&"; }
  std::string operator()(const Pointer& r) const { return std::visit(*this, r.val) + "*"; }
  std::string operator()(const Function& f) const {
    return "(" + join(f.arg_types, ",") + ") -> " + std::visit(*this, f.ret_type);
  }
  std::string operator()(const Closure& c) const {
    return "cls{ " + (*this)(c.fun) + " , " + std::visit(*this, c.captures) + " }";
  }
  std::string operator()(const Array& a) const {
    return "[" + std::visit(*this, a.elem_type) + "x" + std::to_string(a.size) + "]";
  }
  std::string operator()(const Struct& s) const {
    std::string str = "{";
    for (const auto& arg : s.arg_types) {
      str += arg.field + ":" + std::visit(*this, arg.val) + ",";
    }
    return str.substr(0, str.size() - 1) + "}";
  }
  std::string operator()(const Tuple& t) const { return "(" + join(t.arg_types, ",") + ")"; }
  // std::string operator()(const Time& t) const {
  //   return std::visit(*this, t.val) + "@";
  // }
  std::string operator()(const Alias& a) const {
    return a.name + ((verbose) ? ": " + std::visit(*this, a.target) : "");
  }
};

std::string toString(const Value& v, bool verbose = false);
void dump(const Value& v, bool verbose = false);

inline bool isPrimitive(const Value& v) {
  return std::visit(
      [](auto& a) { return std::is_base_of_v<PrimitiveType, std::decay_t<decltype(a)>>; }, v);
}

inline bool isClosure(const Value& v) {
  if (std::holds_alternative<rClosure>(v)) { return true; }
  if (const auto* alias = std::get_if<Box<Alias>>(&v)) {
    return std::holds_alternative<rClosure>(alias->getraw().target);
  }
  return false;
}

}  // namespace types

class TypeEnv {
 private:
  int64_t typeid_count{};

 public:
  TypeEnv() : env() {}
  std::unordered_map<std::string, types::Value> env;

  std::deque<types::Value> tv_container;
  std::shared_ptr<types::TypeVar> createNewTypeVar() {
    auto res = std::make_shared<types::TypeVar>(typeid_count++);
    tv_container.emplace_back(*res);
    return res;
  }
  types::Value& findTypeVar(int tindex) { return tv_container[tindex]; }
  [[nodiscard]] bool exist(std::string key) const { return (env.count(key) > 0); }
  auto begin() { return env.begin(); }
  auto end() { return env.end(); }
  types::Value* tryFind(std::string key) {
    auto it = env.find(key);
    types::Value* res = (it == env.end()) ? nullptr : &it->second;
    return res;
  }
  types::Value& find(std::string const& key) {
    auto* res = tryFind(key);
    if (res == nullptr) {
      throw std::runtime_error("Could not find type for variable \"" + key + "\"");
    }
    return *res;
  }

  auto emplace(std::string key, types::Value typevar) { return env.insert_or_assign(key, typevar); }
  void replaceTypeVars();

  MIMIUM_DLL_PUBLIC std::string toString(bool verbose = false);
  void dump(bool verbose = false);
  void dumpTvLinks();
};

}  // namespace mimium