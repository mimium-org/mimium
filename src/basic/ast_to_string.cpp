/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ast_to_string.hpp"
namespace mimium {

std::ostream& operator<<(std::ostream& os,
                                const newast::Statements& statements){
  StatementStringVisitor svisitor;
  for (const auto&& statement : statements) {
    std::visit(svisitor, statement);
    os << svisitor.getOutput() << "\n";
  }
  os << std::flush;
}

}