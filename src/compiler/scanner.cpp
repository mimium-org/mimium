/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "scanner.hpp"
namespace mimium {

MimiumScanner::MimiumScanner(std::istream& in)
    : yyFlexLexer(in, std::cout), loc(std::make_unique<MimiumParser::location_type>()) {
// debug mode
#ifdef MIMIUM_PARSER_DEBUG
  yy_flex_debug = 1;
#endif
};
}  // namespace mimium