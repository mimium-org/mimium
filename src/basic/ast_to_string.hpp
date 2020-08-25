/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/ast.hpp"

namespace mimium {

enum class Mode { Lisp, Json };

struct ToStringVisitor {
  explicit ToStringVisitor(std::ostream& output, Mode mode = Mode::Lisp);
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
  bool is_prettry;
};
struct StatementStringVisitor;
struct ExprStringVisitor : public ToStringVisitor, public VisitorBase<void> {
  explicit ExprStringVisitor(std::ostream& output, StatementStringVisitor& parent,Mode mode = Mode::Lisp);
  void operator()(const ast::Number& ast);
  void operator()(const ast::String& ast);
  void operator()(const ast::Op& ast);
  void operator()(const ast::Rvar& ast);
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

  void fcallHelper(const ast::Fcall& fcall);

 private:
  StatementStringVisitor& statementstringvisitor;
};

struct StatementStringVisitor : public ToStringVisitor,
                                public VisitorBase<void> {
  explicit StatementStringVisitor(std::ostream& output, Mode mode = Mode::Lisp);
  ExprStringVisitor exprstringvisitor;
  void toString(const ast::ExprPtr expr){
    std::visit(exprstringvisitor,*expr);
  }
  void operator()(const ast::Assign& ast);
  void operator()(const ast::Return& ast);

  void operator()(const ast::Fdef& ast);
  // void operator()(const ast::Declaration& ast);
  void operator()(const ast::Time& ast);
  void operator()(const ast::Fcall& ast);
  void operator()(const ast::For& ast);
};

class AstStringifier {
 public:
 private:
  StatementStringVisitor statement_to_string;
};

template <typename CONTAINER>
inline std::string joinVec(const CONTAINER& vec, const std::string& delim) {
  std::ostringstream stream;
  if (vec.size() > 0) {
    for (auto& elem : vec) {
      if (&elem != &vec[0]) {
        stream << delim;
      }
      if constexpr (std::is_pointer<decltype(elem)>::value ||
                    is_smart_pointer<decltype(elem)>::value) {
        stream << *elem;
      } else {
        stream << elem;
      }
    }
  }
  return stream.str();
}

namespace ast {

inline std::ostream& operator<<(std::ostream& os, const ast::Lvar& lvar){
  StatementStringVisitor evisitor(os);
  auto& format = evisitor.format;
  os << format.lpar << "lvar" << format.delim << lvar.value << format.delim;
  if (lvar.type.has_value()) {
    os << types::toString(lvar.type.value());
  } else {
    os << "unspecified";
  }
  os << format.rpar;
  return os;
}
inline std::ostream& operator<<(std::ostream& os, const ast::Expr& expr){
  StatementStringVisitor stmtvisitor(os, Mode::Lisp);
  std::visit(ExprStringVisitor(os, stmtvisitor, Mode::Lisp), expr);
  return os;
}
template <typename T>
inline std::ostream& operator<<(std::ostream& os,
                                const std::shared_ptr<T> expr) {
  os << *expr;
  return os;
}

// inline std::ostream& operator<<(std::ostream& os,
//                                 const ast::Statement& statement);
inline std::ostream& toString(std::ostream& os,
                              const ast::Statement& statement){
  StatementStringVisitor svisitor(os, Mode::Lisp);
  std::visit(svisitor, statement);
  return os;
}

inline std::ostream& toString(std::ostream& os,
                              ast::Statements const& statements) {
  StatementStringVisitor svisitor(os, Mode::Lisp);
  for (const auto& statement : statements) {
    std::visit(svisitor, *statement);
    os << svisitor.format.br;
  }
  os << std::flush;
  return os;
}
inline std::ostream& operator<<(std::ostream& os,
                                const ast::Statements& statements){
  return toString(os, statements);
}
}  // namespace ast
}  // namespace mimium