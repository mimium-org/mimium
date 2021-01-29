/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
// #define MIMIUM_PARSER_DEBUG
#ifdef MIMIUM_PARSER_DEBUG
#define DEBUG_LEVEL 4
#else
#define DEBUG_LEVEL 0
#endif

#include <filesystem>
#include <fstream>
#include <iostream>

#include "basic/ast.hpp"
namespace mimium {

class MimiumScanner;
class MimiumParser;
// Ast loader class to bridge between parser and ast.
using AstPtr = std::shared_ptr<ast::Statements>;

class Driver {
 public:
 Driver ();
  AstPtr parse(std::istream& is);
  AstPtr parseString(const std::string& source);
  AstPtr parseFile(const std::string& filename);
  void setTopAst(AstPtr top);

 private:
  AstPtr ast_top;
  std::unique_ptr<MimiumParser> parser;
  std::unique_ptr<MimiumScanner> scanner;
};

}  // namespace mimium