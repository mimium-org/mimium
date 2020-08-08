/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#ifndef MIMIUM_DEBUG
#define DEBUG_LEVEL 0
#endif

#include <fstream>
#include <iostream>
#include <filesystem>

#include "basic/ast_new.hpp"
#include "compiler/scanner.hpp"
#include "mimium_parser.hpp"



namespace mimium {

// Ast loader class to bridge between parser and ast.
using AstPtr = std::shared_ptr<newast::Statements>;

class Driver {
 public:
  AstPtr parse(std::istream& is);
  AstPtr parseString(const std::string& source);
  AstPtr parseFile(const std::string& filename);
  void setTopAst(AstPtr top);
 private:
  AstPtr ast_top;
  std::unique_ptr<MimiumParser> parser = nullptr;
  std::unique_ptr<mmmpsr::MimiumScanner> scanner = nullptr;
};

}  // namespace mimium