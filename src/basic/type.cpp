/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "basic/type.hpp"

namespace{
  mimium::types::ToStringVisitor tostrvisitor;//FIXWARNING
}

namespace mimium {
namespace types {

bool isTypeVar(types::Value t) { return std::holds_alternative<Box<types::TypeVar>>(t); }

std::string toString(const Value& v, bool verbose) {
  tostrvisitor.verbose = verbose;
  return std::visit(tostrvisitor, v);
}

void dump(const Value& v, bool verbose) { std::cerr << toString(v, verbose) << "\n"; }

}  // namespace types

}  // namespace mimium