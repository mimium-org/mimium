/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/ast.hpp"

namespace mimium {

enum class Mode { Lisp, Json };

struct ToStringVisitor {
  MIMIUM_DLL_PUBLIC explicit ToStringVisitor(std::ostream& output, Mode mode = Mode::Lisp);
  std::ostream& output;
  struct {
    std::string lpar;
    std::string rpar;
    // angle brackets
    std::string lpar_a;
    std::string rpar_a;
    std::string delim;
    std::string br;
  } format;

 private:
  Mode mode;
  // bool is_prettry;
};

struct AstStringifier : public ToStringVisitor {
  explicit AstStringifier(std::ostream& output, Mode mode = Mode::Lisp):ToStringVisitor(output,mode){};
  void operator()(const ast::Number& ast);
  void operator()(const ast::String& ast);
  void operator()(const ast::Op& ast);
  void operator()(const ast::Symbol& ast);
  void operator()(const ast::Self& ast);
  void operator()(const ast::Lambda& ast);
  void operator()(const ast::Fcall& ast);
  void operator()(const ast::If& ast);
  void operator()(const ast::Struct& ast);
  void operator()(const ast::StructAccess& ast);
  void operator()(const ast::ArrayInit& ast);
  void operator()(const ast::ArrayAccess& ast);
  void operator()(const ast::Tuple& ast);
  void operator()(const ast::Block& ast);

  void operator()(const ast::ArrayLvar& ast);
  void operator()(const ast::TupleLvar& ast);
  void operator()(const ast::DeclVar& ast);

  void operator()(const ast::Assign& ast);
  void operator()(const ast::TypeAssign& ast);
  void operator()(const ast::Return& ast);
  void operator()(const ast::Fdef& ast);
  void operator()(const ast::Time& ast);
  void operator()(const ast::For& ast);
  // variants
  MIMIUM_DLL_PUBLIC void toString(const ast::Lvar& ast);
  MIMIUM_DLL_PUBLIC void toString(const ast::Expr& ast);
  MIMIUM_DLL_PUBLIC void toString(ast::ExprPtr ast);
  MIMIUM_DLL_PUBLIC void toString(const ast::Statement& ast);
  MIMIUM_DLL_PUBLIC void toString(const ast::Statements& ast);

  template <typename T>
  void toStringVec(const T& vec) {
    if (vec.size() > 0) {
      for (auto& elem : vec) {
        if (&elem != &vec[0]) { output << format.delim; }
        toString(elem);
      }
    }
  }
};

template <typename CONTAINER>
inline std::string joinVec(const CONTAINER& vec, const std::string& delim) {
  std::ostringstream stream;
  if (vec.size() > 0) {
    for (auto& elem : vec) {
      if (&elem != &vec[0]) { stream << delim; }
      stream << elem;
    }
  }
  return stream.str();
}

namespace ast {

inline std::ostream& operator<<(std::ostream& os, ast::Expr const& val) {
  AstStringifier visitor(os);
  visitor.toString(val);
  return os;
}
inline std::ostream& operator<<(std::ostream& os, ast::Statement const& val) {
  AstStringifier visitor(os);
  visitor.toString(val);
  return os;
}
inline std::ostream& operator<<(std::ostream& os, ast::Statements const& val) {
  AstStringifier visitor(os);
  visitor.toString(val);
  return os;
}
inline std::ostream& operator<<(std::ostream& os, ast::Lvar const& val) {
  AstStringifier visitor(os);
  visitor.toString(val);
  return os;
}

}  // namespace ast
}  // namespace mimium