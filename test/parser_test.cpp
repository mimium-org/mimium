#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
#include "basic/ast_new.hpp"
#include "compiler/ast_loader_new.hpp"
namespace mimium{

#define PARSER_TEST(FILENAME)\
TEST(parser, FILENAME) {\
Driver driver{};\
auto ast = driver.parseFile("parser/"#FILENAME".mmm");}

PARSER_TEST(number)

PARSER_TEST(lvar_type)

}