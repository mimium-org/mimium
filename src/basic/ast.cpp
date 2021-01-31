
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ast.hpp"

namespace mimium::ast {
const static std::map<OpId, std::string_view> op_str = {{OpId::Add, "Add"},
                                                 {OpId::Sub, "Sub"},
                                                 {OpId::Mul, "Mul"},
                                                 {OpId::Div, "Div"},
                                                 {OpId::Mod, "Mod"},
                                                 {OpId::Exponent, "Exponent"},
                                                 {OpId::Equal, "Equal"},
                                                 {OpId::NotEq, "NotEq"},
                                                 {OpId::LessEq, "LessEq"},
                                                 {OpId::GreaterEq, "GreaterEq"},
                                                 {OpId::LessThan, "LessThan"},
                                                 {OpId::GreaterThan, "GreaterThan"},
                                                 {OpId::And, "And"},
                                                 {OpId::BitAnd, "BitAnd"},
                                                 {OpId::Or, "Or"},
                                                 {OpId::BitOr, "BitOr"},
                                                 {OpId::Xor, "Xor"},
                                                 {OpId::Not, "Not"},
                                                 {OpId::LShift, "LShift"},
                                                 {OpId::RShift, "RShift"}};

std::string_view getOpString(OpId id){
    return op_str.at(id);
}

}  // namespace mimium::ast