/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "compiler/ast_loader.hpp"
#include "basic/filereader.hpp"
#include "compiler/scanner.hpp"
#include "mimium_parser.hpp"

namespace mimium {
Driver::Driver() : parser(nullptr), scanner(nullptr) {}
Driver::~Driver() = default;

Driver::expr Driver::parse(std::istream& is) {
  scanner = std::make_unique<MimiumScanner>(is);
  parser = std::make_unique<MimiumParser>(*scanner, *this);
  parser->set_debug_level(DEBUG_LEVEL);  // debug
  int res = 0;
  try {
    res = parser->parse();
  } catch (std::exception& e) { throw std::runtime_error(e.what()); } catch (...) {
    throw std::runtime_error("undefined parse error");
  }
  if (res > 0) { throw std::runtime_error("parse error."); }
  assert(toplevel.has_value());
  return lowerToplevel(this->toplevel.value());
}

Driver::expr Driver::parseString(const std::string& source) {
  std::stringbuf buf(source);
  std::istream is(&buf);
  return parse(is);
}
Driver::expr Driver::parseFile(const std::string& filename) {
  FileReader reader(fs::current_path());
  auto src = reader.loadFile(filename);
  return parseString(src.source);
}

void Driver::setTopLevel(TopLevel::Expression const& toplevel) { this->toplevel = toplevel; }

std::optional<Driver::expr> Driver::processCompilerDirectives(
    TopLevel::CompilerDirective const& e) {
  using res_t = std::optional<Driver::expr>;
  auto&& vis = overloaded{
      [&](TopLevel::TypeAlias const& t) -> res_t {
        alias_map.emplace(t.first, t.second);
        return std::nullopt;
      },
      [&](TopLevel::Import const& t) -> res_t { return std::optional(parseFile(t.path)); }};
  return std::visit(vis, e);
}

Driver::expr Driver::lowerToplevel(TopLevel::Expression const& e) {
  using res_t = std::optional<Hast::Statement>;
  Hast::Block res;
  for (const auto& line : e) {
    auto s =
        std::visit(overloaded{[&](TopLevel::CompilerDirective const& a) -> res_t {
                                processCompilerDirectives(a);
                                return std::nullopt;
                              },
                              [](Hast::Statement const& e) -> res_t { return std::optional(e); }},
                   line);
    if (s) { res.v.statements.emplace_back(s.value()); }
  }
  return Hast::expr{res};
}

}  // namespace mimium