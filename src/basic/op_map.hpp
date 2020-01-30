/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <unordered_map>
#include "mimium_parser.hpp"

using tokentype = mmmpsr::MimiumParser::token::yytokentype;

static std::unordered_map<std::string, tokentype> op_map {
    {"+",mmmpsr::MimiumParser::token::ADD},
    {"-",mmmpsr::MimiumParser::token::SUB},
    {"*",mmmpsr::MimiumParser::token::MUL},
    {"/",mmmpsr::MimiumParser::token::DIV},
    {"^",mmmpsr::MimiumParser::token::EXPONENT},
    {"%",mmmpsr::MimiumParser::token::MOD},
    {"&",mmmpsr::MimiumParser::token::AND},
    {"|",mmmpsr::MimiumParser::token::OR},
    {"&&",mmmpsr::MimiumParser::token::BITAND},
    {"||",mmmpsr::MimiumParser::token::BITAND},
    {"==",mmmpsr::MimiumParser::token::EQ},
    {"!=",mmmpsr::MimiumParser::token::NEQ}
};
