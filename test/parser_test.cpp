#include "gtest/gtest.h"
#include "../parser.hpp"

TEST(parser_test, digit) {
    auto testparser = pc::parser(digit);
    std::string line= "123";
    auto res = testparser.parse(line).success().value();
    EXPECT_EQ(123, res);
}

TEST(parser_test, num) {
    auto testparser = pc::parser(num);
    std::string line= "123";
    auto res = testparser.parse(line).success().value();
    EXPECT_EQ(123, res.getVal());
}

TEST(parser_test, symbol) {
    auto testparser = pc::parser(symbol);
    std::string line= "hoge";
    auto res = testparser.parse(line).success().value();
    EXPECT_EQ("hoge", res.getVal());
}

TEST(parser_test, expr) {
    auto testparser = pc::parser(expr);
    std::string line= "1+24";
    auto res = testparser.parse(line).success().value();
    EXPECT_EQ("+", res.getOp());
    EXPECT_EQ(NumberID, res.getLHS()->getValueID());
    EXPECT_EQ(NumberID, res.getRHS()->getValueID());

}