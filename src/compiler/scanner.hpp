/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once


#if !defined(yyFlexLexerOnce)
#include "FlexLexer.h"
#endif

#include "mimium_parser.hpp"

namespace mmmpsr {

class MimiumScanner : public yyFlexLexer {
 public:
  explicit MimiumScanner(std::istream &in)
      : yyFlexLexer(in, std::cout),
        loc(std::make_unique<MimiumParser::location_type>()){

        };

   ~MimiumScanner()override = default;

  // get rid of override virtual function warning
  using FlexLexer::yylex;

  virtual int yylex(MimiumParser::semantic_type * lval,
                    MimiumParser::location_type *location);
  // YY_DECL defined in mc_lexer.l
  // Method body created by flex in mc_lexer.yy.cc

 private:
  /* yyval ptr */
  MimiumParser::semantic_type *yylval = nullptr;
  std::shared_ptr<MimiumParser::location_type> loc = nullptr;
};

}  // namespace mmmpsr
