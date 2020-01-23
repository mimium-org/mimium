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
template <typename T>
struct recursive_wrapper {
  // construct from an existing object
  recursive_wrapper(T t_) {
    t.reserve(1);
    t.emplace_back(std::move(t_));
  }  // NOLINT

  // cast back to wrapped type
  // operator T&() { return t.front(); }              // NOLINT
  operator const T&() const { return t.front(); }  // NOLINT

  T& getraw() { return t.front(); }
  // store the value
  std::vector<T> t;
};

template <typename T>
inline bool operator==(const recursive_wrapper<T>& t1,
                       const recursive_wrapper<T>& t2) {
  return t1.t.front() == t2.t.front();
}
template <typename T>
inline bool operator!=(const recursive_wrapper<T>& t1,
                       const recursive_wrapper<T>& t2) {
  return !(t1 == t2);
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

struct TypeVar {
  explicit TypeVar(int i) : index(i) {}
  int index;
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

struct Ref;
struct Pointer;
struct Function;
struct Closure;
struct Array;
struct Struct;
struct Tuple;
struct Time;

struct Alias;

using Value =
    std::variant<None, TypeVar, Void, Float, String, recursive_wrapper<Ref>,
                 recursive_wrapper<Pointer>, recursive_wrapper<Function>,
                 recursive_wrapper<Closure>, recursive_wrapper<Array>,
                 recursive_wrapper<Struct>, recursive_wrapper<Tuple>,
                 recursive_wrapper<Time>, recursive_wrapper<Alias>>;

struct ToStringVisitor;

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
struct Time : Aggregate {
  Value val;
  Float time;
};
inline bool operator==(const Time& t1, const Time& t2) {
  return t1.val == t2.val;
}
inline bool operator!=(const Time& t1, const Time& t2) {
  return t1.val != t2.val;
}
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
  std::vector<Value> arg_types;
  Value ret_type;

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
  Function fun;
  Value captures;
  explicit Closure(Function fun, Value captures)
      : fun(std::move(fun)), captures(std::move(captures)){};
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
  explicit Array(Value elem, int size)
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
[[maybe_unused]] static bool isTypeVar(types::Value t) {
  return std::holds_alternative<types::TypeVar>(t);
}

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
    return "Closure:{ " + (*this)(c.fun) +" , " +std::visit(*this, c.captures) +" }";
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
  std::string operator()(const Time& t) const {
    return std::visit(*this, t.val) + "@";
  }
  std::string operator()(const Alias& a) const { 
    return a.name +( (verbose)? ": "+std::visit(*this,a.target):"");
 }
};
static ToStringVisitor tostrvisitor;
std::string toString(const Value& v);

struct KindVisitor {
  template <class T>
  Kind operator()(T t) {
    return T::kind;
  }
  template <class T>
  Kind operator()(recursive_wrapper<T> t) {
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
  std::unordered_map<std::string, types::Value> env;

 public:
  TypeEnv() : env() {}
  types::TypeVar createNewTypeVar() { return types::TypeVar(typeid_count++); }
  bool exist(std::string key) { return (env.count(key) > 0); }
  types::Value& find(std::string key) {
    auto res = env.find(key);
    if (res == env.end()) {
      throw std::logic_error("Could not find type for variable \"" + key +
                             "\"");
    }
    return res->second;
  }

  auto emplace(std::string key, types::Value typevar) {
    return env.insert_or_assign(key, typevar);
  }

  std::string dump() {
    std::stringstream ss;
    types::ToStringVisitor vis;
    vis.verbose=true;
    for (auto& [key, val] : env) {
      ss << key << " : " << std::visit(vis,val) <<"\n";
    }
    return ss.str();
  }
};

}  // namespace mimium