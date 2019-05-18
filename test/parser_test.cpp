#include "gtest/gtest.h"
#include "../parser.hpp"

namespace tch{
    using s  = std::string;
    s num = "123";
    s str = "hoge";
    s exprnumnum = "1+2";
    s exprnumstr = "1+symn";

};

TEST(parser_test, digit) {
    auto testparser = pc::parser(digit);
    auto res = testparser.parse(tch::num).success().value();
    auto res2 = testparser.parse(tch::str);
    EXPECT_EQ('1', res);
    EXPECT_FALSE(res2.is_success());
}

TEST(parser_test, num) {
    auto testparser = pc::parser(num);
    auto res = testparser.parse(tch::num).success().value();
    auto res2 = testparser.parse(tch::str);

    EXPECT_EQ(123, res.getVal());
    EXPECT_EQ(NumberID, res.getValueID());
    EXPECT_FALSE(res2.is_success());

}

TEST(parser_test, symbol) {
    auto testparser = pc::parser(symbol);
    auto res = testparser.parse(tch::str).success().value();
    auto res2 = testparser.parse(tch::num);

    EXPECT_EQ("hoge", res.getVal());
    EXPECT_EQ(VariableID, res.getValueID());
    EXPECT_FALSE(res2.is_success());
}


TEST(parser_test, expr) {
    auto testparser = pc::parser(expr);
    auto res = testparser.parse(tch::exprnumnum).success().value();
    BaseAST* lhsbase= res.getLHS(); 
    NumberAST* lhs = dynamic_cast<NumberAST*>(lhsbase);
    NumberAST* rhs = dynamic_cast<NumberAST*>(res.getRHS());
    EXPECT_EQ("+", res.getOp());
    EXPECT_EQ(NumberID, lhs->getValueID());
    EXPECT_EQ(NumberID,  rhs->getValueID());
    
    // auto res2 = testparser.parse(tch::exprnumstr).success().value();

    // EXPECT_EQ("+", res2.getOp());
    // EXPECT_EQ(NumberID, res2.getLHS()->getValueID());
    // EXPECT_EQ(VariableID, res2.getRHS()->getValueID());
}
