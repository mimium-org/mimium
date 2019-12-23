#pragma once

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

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
  operator const T&() const { return t.front(); }  // NOLINT
  // store the value
  std::vector<T> t;
  std::string toString() { return t.front().toString(); };
};
template <typename T>
inline bool operator==(const recursive_wrapper<T>& t1,
                       const recursive_wrapper<T>& t2) {
  return t1.t.front() == t2.t.front();
}
template <typename T>
inline bool operator!=(const recursive_wrapper<T>& t1,
                       const recursive_wrapper<T>& t2) {
  return !(t1==t2);
}
namespace types {
struct None {
  std::string toString() { return "None"; }
};
inline bool operator==(const None& t1, const None& t2) { return true; };
inline bool operator!=(const None& t1, const None& t2) { return false; }
struct TypeVar {
  TypeVar(int i) : index(i) {}
  int index;
  int getIndex() { return index; }
  void setIndex(int newindex) { index = newindex; }
  std::string toString() {
    return "Not Unified Type Variable:" + std::to_string(index);
  }
};
inline bool operator==(const TypeVar& t1, const TypeVar& t2) { return t1.index==t2.index; }
inline bool operator!=(const TypeVar& t1, const TypeVar& t2) { return t1.index!=t2.index; }
struct Void {
  std::string toString() { return "Void"; }
};
inline bool operator==(const Void& t1, const Void& t2) { return true; };
inline bool operator!=(const Void& t1, const Void& t2) { return false; }
struct Float {
  std::string toString() { return "Float"; };
};
inline bool operator==(const Float& t1, const Float& t2) { return true; };
inline bool operator!=(const Float& t1, const Float& t2) { return false; }
struct String {
  std::string toString() { return "String"; };
};
inline bool operator==(const String& t1, const String& t2) { return true; };
inline bool operator!=(const String& t1, const String& t2) { return false; }

struct Function;
struct Array;
struct Struct;
struct Time;
using Value = std::variant<
    types::None, types::TypeVar, types::Void, types::Float, types::String,
    recursive_wrapper<types::Function>, recursive_wrapper<types::Array>,
    recursive_wrapper<types::Struct>, recursive_wrapper<types::Time>>;
struct Time {
  Value val;
  Float time;
  Time() = default;
  std::string toString() {
    return std::visit([](auto c) { return c.toString(); }, val);
    +"@" + time.toString();
  }
};
inline bool operator==(const Time& t1, const Time& t2) { return t1.val==t2.val; }
inline bool operator!=(const Time& t1, const Time& t2) { return t1.val!=t2.val; }
struct Function {
  Function() = default;
  Function(std::vector<Value> arg_types_p, Value ret_type_p)
      : arg_types(std::move(arg_types_p)), ret_type(std::move(ret_type_p)){};
  void init(std::vector<Value> arg_types_p, Value ret_type_p) {
    arg_types = std::move(arg_types_p);
    ret_type = std::move(ret_type_p);
  }
  std::vector<Value> arg_types;
  Value ret_type;
  Value& getReturnType() { return ret_type; }
  std::vector<Value>& getArgTypes() { return arg_types; }
  std::string toString() {
    std::string s = "Fn[ (";
    int count = 1;
    for (const auto& v : arg_types) {
      s += std::visit([](auto c) { return c.toString(); }, v);
      if (count < arg_types.size()) s += " , ";
      count++;
    }
    s += ") -> " + std::visit([](auto c) { return c.toString(); }, ret_type) +
         " ]";
    return s;
  };
  template <class... Args>
  static std::vector<Value> createArgs(Args&&... args) {
    std::vector<Value> a = {std::forward<Args>(args)...};
    return a;
  }
};
inline bool operator==(const Function& t1, const Function& t2) { return t1.ret_type==t2.ret_type && t1.arg_types==t2.arg_types; }
inline bool operator!=(const Function& t1, const Function& t2) { return !(t1==t2); }
struct Array {
  Value elem_type;
  explicit Array(Value elem) : elem_type(std::move(elem)) {}
  std::string toString() {
    return "array[" +
           std::visit([](auto c) { return c.toString(); }, elem_type) + "]";
  }
  Value getElemType() { return elem_type; }
};
inline bool operator==(const Array& t1, const Array& t2) { return t1.elem_type==t2.elem_type; }
inline bool operator!=(const Array& t1, const Array& t2) { return t1.elem_type!=t2.elem_type; }
struct Struct {
  std::vector<Value> arg_types;
  explicit Struct(std::vector<Value> types) : arg_types(std::move(types)) {}
  std::string toString() {
    std::string s;
    s += "struct{";
    for (auto& a : arg_types) {
      s += std::visit([](auto c) { return c.toString(); }, a) + " ";
    }
    s += "}";
    return s;
  }
};
inline bool operator==(const Struct& t1, const Struct& t2) {
  return (t1.arg_types == t2.arg_types);
};
inline bool operator!=(const Struct& t1, const Struct& t2) {
  return !(t1 == t2);
}

static bool isTypeVar(types::Value t) {
  return std::holds_alternative<types::TypeVar>(t);
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
      ss << key << " : " << std::visit([](auto t) { return t.toString(); }, val)
         << std::endl;
    }
    return ss.str();
  }
};
}  // namespace mimium