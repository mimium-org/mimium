/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "compiler/ast_loader.hpp"
#include "basic/ast_to_string.hpp"

namespace fs = std::filesystem;


namespace mimium {

AstPtr Driver::parse(std::istream& is) {
  scanner = std::make_unique<mmmpsr::MimiumScanner>(is);
  parser = std::make_unique<MimiumParser>(*scanner, *this);
  parser->set_debug_level(DEBUG_LEVEL);  // debug
  int res =parser->parse();
  return ast_top;
}

AstPtr Driver::parseString(const std::string& source) {
  std::stringbuf buf(source);
  std::istream is(&buf);
  return std::move(parse(is));
}
AstPtr Driver::parseFile(const std::string& filename) {
  fs::path path(filename);
  auto abspath = fs::absolute(path);
  auto ext = path.extension().string();

  if (ext != ".mmm") {
    throw std::runtime_error(
        "file type " + ext +
        " does not match to mimium source code file(.mmm).");
  }
  std::error_code ec;
  auto status = fs::status(abspath,ec);
  //memo: fs::exists(path,ec) for .mmm file returns file type of "unknown", not "regular" or "none". to prevent error, need to check specifically not to be "not found"
  if (status.type() ==fs::file_type::none ||status.type() ==fs::file_type::not_found) {
    throw std::runtime_error("file " + abspath.string() + " does not exist.");
  }
  std::ifstream ifs;
  ifs.open(abspath);
  ifs.exceptions(std::fstream::failbit | std::fstream::badbit);
  return std::move(parse(ifs));
}

void Driver::setTopAst(AstPtr top){
  this->ast_top = top;
}


}  // namespace mimium