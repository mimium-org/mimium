#include "basic/ast.hpp"
#include "basic/ast_to_string.hpp"
#include "compiler/ast_loader.hpp"
#include "mimium_parser.hpp"
#include "compiler/scanner.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
namespace mimium {
// NOLINTNEXTLINE
#define PARSER_TEST(FILENAME)                                     \
  TEST(parser, FILENAME) { /*NOLINT*/                             \
    Driver driver{};                                              \
    *driver.parseFile(TEST_ROOT_DIR "/parser/" #FILENAME ".mmm"); \
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
PARSER_TEST(tupletype)
PARSER_TEST(typedecl)
PARSER_TEST(structaccess)

TEST(parser, ast_complete) {//NOLINT
  Driver driver{};
  auto ast = driver.parseFile(TEST_ROOT_DIR "/ast_complete.mmm");
  // std::cerr << ast << "\n";
}

}  // namespace mimium