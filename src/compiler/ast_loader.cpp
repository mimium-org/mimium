/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "compiler/ast_loader.hpp"
#include "basic/ast_to_string.hpp"
#include "basic/filereader.hpp"

namespace fs = std::filesystem;

namespace mimium {

AstPtr Driver::parse(std::istream& is) {
  scanner = std::make_unique<mmmpsr::MimiumScanner>(is);
  parser = std::make_unique<MimiumParser>(*scanner, *this);
  parser->set_debug_level(DEBUG_LEVEL);  // debug
  int res=0;
  try{
    res = parser->parse();
  }catch(std::exception& e){
    throw std::runtime_error(e.what());
  }catch(...){
    throw std::runtime_error("undefined parse error");;
  }
  if(res>0){
    throw std::runtime_error("parse error.");
  }
  return ast_top;
}

AstPtr Driver::parseString(const std::string& source) {
  std::stringbuf buf(source);
  std::istream is(&buf);
  return std::move(parse(is));
}
AstPtr Driver::parseFile(const std::string& filename) {
  FileReader reader(fs::current_path());
  auto src = reader.loadFile(filename);
  return std::move(parseString(src.source));
}

void Driver::setTopAst(AstPtr top) { this->ast_top = top; }

}  // namespace mimium