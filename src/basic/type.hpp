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
  constexpr static bool is_aggregatewrapper = true;
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
  constexpr static bool is_aggregate = false;
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
  constexpr static bool is_aggregate = false;
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
using Value =
    std::variant<std::monostate, None, TypeVar, Void, Float, String,
                 recursive_wrapper<Ref>,recursive_wrapper<Pointer>,
                  recursive_wrapper<Function>
                 ,recursive_wrapper<Closure>
                  ,recursive_wrapper<Array>
                 , recursive_wrapper<Struct>
                 , recursive_wrapper<Tuple>, recursive_wrapper<Time>
                 >;

// // helper function that can get recursive content from variant
// template <class T>
// struct RecVar {
//   static auto get(Value& v) {
//     if constexpr (T::is_aggregate) {
//       return std::get<recursive_wrapper<T>>(v).getraw();
//     } else {
//       return std::get<T>(v);
//     }
//   }
//   static bool checkType(Value& v){
//     if constexpr (T::is_aggregate){
//       return std::holds_alternative<recursive_wrapper<T>>(v);
//     }else{
//       return std::holds_alternative<T>(v);
//     }
//   }
// };
struct ToStringVisitor;

struct Ref {
  Ref()=default;
  explicit Ref(Value v):val(std::move(v)){};
  Value val;
};

inline bool operator==(const Ref& t1, const Ref& t2) {
  return t1.val == t2.val;
}
inline bool operator!=(const Ref& t1, const Ref& t2) {
  return t1.val != t2.val;
}
struct Pointer {
  Value val;
};
inline bool operator==(const Pointer& t1, const Pointer& t2) {
  return t1.val == t2.val;
}
inline bool operator!=(const Pointer& t1, const Pointer& t2) {
  return t1.val != t2.val;
}
struct Time {
  Value val;
  Float time;
  constexpr static bool is_aggregate = true;
};
inline bool operator==(const Time& t1, const Time& t2) {
  return t1.val == t2.val;
}
inline bool operator!=(const Time& t1, const Time& t2) {
  return t1.val != t2.val;
}
struct Function {
  Function() =default;
  explicit Function(Value ret_type_p, std::vector<Value> arg_types_p)
      : arg_types(std::move(arg_types_p)), ret_type(std::move(ret_type_p)){};
  void init(std::vector<Value> arg_types_p, Value ret_type_p) {
    arg_types = std::move(arg_types_p);
    ret_type = std::move(ret_type_p);
  }
  std::vector<Value> arg_types;
  Value ret_type;
  constexpr static bool is_aggregate = true;

  Value& getReturnType() { return ret_type; }
  std::vector<Value>& getArgTypes() { return arg_types; }

  static Function create(const types::Value& ret,
                         const std::vector<types::Value>& arg) {
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

struct Closure {
  Value fun;
  std::string context;
  explicit Closure(Value fun,std::string context):fun(std::move(fun)),context(std::move(context)){}
};
inline bool operator==(const Closure& t1, const Closure& t2) {
  return t1.fun == t2.fun && t1.context == t2.context;
}
inline bool operator!=(const Closure& t1, const Closure& t2) {
  return !(t1 == t2);
}
struct Array {
  Value elem_type;
  int size;
  constexpr static bool is_aggregate = true;
  explicit Array(Value elem) : elem_type(std::move(elem)) {}
};
inline bool operator==(const Array& t1, const Array& t2) {
  return( t1.elem_type == t2.elem_type)&&(t1.size==t2.size);
}
inline bool operator!=(const Array& t1, const Array& t2) {
  return !(t1==t2);
}
struct Tuple {
  std::vector<Value> arg_types;
  constexpr static bool is_aggregate = true;
  explicit Tuple(std::vector<Value> types) : arg_types(std::move(types)) {}
};

inline bool operator==(const Tuple& t1, const Tuple& t2) {
  return (t1.arg_types == t2.arg_types);
};
inline bool operator!=(const Tuple& t1, const Tuple& t2) {
  return !(t1 == t2);
}
struct Struct {
  struct Keytype {
    std::string field;
    Value val;
  };
  std::vector<Keytype> arg_types;
  constexpr static bool is_aggregate = true;
  explicit Struct(std::vector<Keytype> types) : arg_types(std::move(types)) {}
  explicit operator Tuple(){ 
    std::vector<Value> res;
    std::for_each(arg_types.begin(),arg_types.end(), [&](const auto& c){res.push_back(c.val);});
    return Tuple(res);
  }
  explicit operator const Tuple()const{ 
    std::vector<Value> res;
    std::for_each(arg_types.begin(),arg_types.end(), [&](const auto& c){res.push_back(c.val);});
    return Tuple(res);
  }
};
inline bool operator==(const Struct::Keytype& t1, const Struct::Keytype& t2) {
  return (t1.field == t2.field)&&(t1.val == t2.val);
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

[[maybe_unused]] static bool isTypeVar(types::Value t) {
  return std::holds_alternative<types::TypeVar>(t);
}

struct ToStringVisitor {
  [[nodiscard]] std::string join(const std::vector<types::Value>& vec,
                                 std::string delim) const {
    return std::accumulate(std::next(vec.begin()), vec.end(),
                           std::visit(*this, *vec.begin()),
                           [&](std::string a, const Value& b) {
                             return std::move(a) + delim + std::visit(*this, b);
                           });
  };
  std::string operator()(std::monostate) const { return ""; }
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
  std::string operator()(const Closure& c)const {
    return "Closure:" + std::visit(*this, c.fun);
  }
  std::string operator()(const Array& a) const {
    return "["+std::visit(*this, a.elem_type)+"x" + std::to_string(a.size)+ "]";
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
};
static ToStringVisitor tostrvisitor;
static std::string toString(const Value& v){
  return std::visit(tostrvisitor,v);
}

}  // namespace types

class TypeEnv {
 private:
  int64_t typeid_count;
  std::unordered_map<std::string, types::Value> env;

 public:
  TypeEnv() : typeid_count(0), env() {}
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

  auto emplace(std::string_view key, types::Value typevar) {
    return env.emplace(key, typevar);
  }

  std::string dump() {
    std::stringstream ss;
    for (auto& [key, val] : env) {
      ss << key << " : " << types::toString(val)
         << "\n";
    }
    return ss.str();
  }
};
}  // namespace mimium