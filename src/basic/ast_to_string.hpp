/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/ast_new.hpp"

namespace mimium {
struct ToStringVisitor {
  std::string getOutput() { return output.str(); }

 private:
  std::stringstream output;
};

struct TermStringVisitor : public ToStringVisitor {
  void operator()(newast::Number ast);
  void operator()(newast::String ast);
  void operator()(newast::Op ast);
  void operator()(newast::Rvar ast);
  void operator()(newast::Self ast);
  void operator()(Rec_Wrap<newast::Lambda> ast);
  void operator()(Rec_Wrap<newast::Fcall> ast);
  void operator()(Rec_Wrap<newast::Time> ast);
  void operator()(Rec_Wrap<newast::StructAccess> ast);
  void operator()(Rec_Wrap<newast::ArrayInit> ast);
  void operator()(Rec_Wrap<newast::ArrayAccess> ast);
  void operator()(Rec_Wrap<newast::Tuple> ast);
};

struct StatementStringVisitor : public ToStringVisitor {
  void operator()(newast::Assign ast);
  void operator()(newast::Return ast);
  void operator()(newast::Declaration ast);
  void operator()(Rec_Wrap<newast::Fdef> ast);
  void operator()(Rec_Wrap<newast::For> ast);
  void operator()(Rec_Wrap<newast::If> ast);
  void operator()(Rec_Wrap<newast::Expr> ast);
};

class AstStringifier {
 public:
 private:
  TermStringVisitor term_to_string;
  StatementStringVisitor statement_to_string;
};
namespace newast {}

inline std::ostream& operator<<(std::ostream& os,
                                const newast::Statements& statements);

}  // namespace mimium