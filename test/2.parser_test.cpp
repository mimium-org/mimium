#include "basic/ast.hpp"
#include "basic/ast_to_string.hpp"
#include "compiler/ast_loader.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
namespace mimium {

#define PARSER_TEST(FILENAME)                                             \
  TEST(parser, FILENAME) {                                                \
    Driver driver{};                                                      \
    ast::Statements& ast = *driver.parseFile("parser/" #FILENAME ".mmm"); \
  }

PARSER_TEST(operators)

PARSER_TEST(number)
PARSER_TEST(lvar_type)
PARSER_TEST(ifstmt)
PARSER_TEST(ifexpr)
PARSER_TEST(lambda)
PARSER_TEST(pipeline)
PARSER_TEST(fdef)
PARSER_TEST(self)
PARSER_TEST(array)
PARSER_TEST(tuplelvar)
PARSER_TEST(tuple)

TEST(parser, ast_complete) {
  Driver driver{};
  auto ast = driver.parseFile("ast_complete.mmm");
  // std::cerr << ast << "\n";
}

}  // namespace mimium