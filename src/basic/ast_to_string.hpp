/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/ast_new.hpp"

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

struct ExprStringVisitor : public ToStringVisitor,public VisitorBase<void> {
  explicit ExprStringVisitor(std::ostream& output, Mode mode = Mode::Lisp);

  void operator()(const newast::Number& ast);
  void operator()(const newast::String& ast);
  void operator()(const newast::Op& ast);
  void operator()(const newast::Rvar& ast);
  void operator()(const newast::Self& ast);
  void operator()(const newast::Lambda& ast);
  void operator()(const newast::Fcall& ast);
  void operator()(const newast::Time& ast);
  void operator()(const newast::Struct& ast);
  void operator()(const newast::StructAccess& ast);
  void operator()(const newast::ArrayInit& ast);
  void operator()(const newast::ArrayAccess& ast);
  void operator()(const newast::Tuple& ast);

 private:
  void fcallHelper(const newast::Fcall& fcall);
};

struct StatementStringVisitor : public ToStringVisitor,public VisitorBase<void> {
  explicit StatementStringVisitor(std::ostream& output, Mode mode = Mode::Lisp);

  ExprStringVisitor exprstringvisitor;
  void operator()(const newast::Assign& ast);
  void operator()(const newast::Return& ast);
  // void operator()(const newast::Declaration& ast);
  void operator()(const newast::For& ast);
  void operator()(const newast::If& ast);
  void operator()(const newast::ExprPtr& ast);
};

class AstStringifier {
 public:
 private:
  StatementStringVisitor statement_to_string;
};

template <typename CONTAINER>
inline std::string joinVec(const CONTAINER& vec, const std::string& delim) {
  std::ostringstream stream;
  if(vec.size()>0){
  for (auto& elem : vec) {
    if (&elem != &vec[0]) {
      stream << delim;
    }
    if constexpr(std::is_pointer<decltype(elem)>::value || is_smart_pointer<decltype(elem)>::value){
    stream << *elem;
    }else{
    stream << elem;
    }
  }
  }
  return stream.str();
}

namespace newast {

inline std::ostream& operator<<(std::ostream& os, const newast::Lvar& lvar) ;
inline std::ostream& operator<<(std::ostream& os, const newast::Expr& expr);

template<typename T>
inline std::ostream& operator<<(std::ostream& os, const std::shared_ptr<T> expr){
  os << *expr;
  return os;
}

// inline std::ostream& operator<<(std::ostream& os,
//                                 const newast::Statement& statement);
std::ostream& toString(std::ostream& os,
                                 const newast::Statement& statement);

std::ostream& toString(std::ostream& os,
                                  newast::Statements const& statements);

inline std::ostream& operator<<(std::ostream& os,
                                const newast::Statements& statements){
  return toString(os,statements);
}
}  // namespace newast
}  // namespace mimium