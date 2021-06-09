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

#include <fstream>
#include <iostream>
#include "utils/include_filesystem.hpp"

#include "basic/ast_new.hpp"
namespace mimium {

class MimiumScanner;
class MimiumParser;
// Ast loader class to bridge between parser and ast.

class Driver {
  using expr = TopLevel::expr;

 public:
  Driver();
  ~Driver();
  expr parse(std::istream& is);
  expr parseString(const std::string& source);
  expr parseFile(const std::string& filename);
  //main interface for getting ast from bison.
  void setTopLevel(TopLevel::Expression const& toplevel);
 private:
  std::unique_ptr<MimiumParser> parser;
  std::unique_ptr<MimiumScanner> scanner;
  std::optional<TopLevel::Expression> toplevel;
  TopLevel::AliasMap_t alias_map;
  std::optional<expr> processCompilerDirectives(TopLevel::CompilerDirective const& e);
  expr lowerToplevel(TopLevel::Expression const& e);
};

}  // namespace mimium