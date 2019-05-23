#include "gtest/gtest.h"
#include "parser.hpp"

namespace tch{
    using s  = std::string;
    s num = "123";
    s str = "hoge";
    s list ="symb, hoge,123";
    s exprnumnum = "1+2";
    s exprnumnumnum = "1+2+3";
    s exprnumstr = "1+symn";
    s fcall = "myfunc(these,are,args , 1,2, 3)";
    s assign  ="test = hoge + 1";
    s fdef  ="test(x)=hoge+1+x*y";
    s lambda = "(x,y,z)=>{x+y+z}";
    s top = "test(x)=hoge+1+x*y;test = hoge + 1;test(x)=hoge+1+x*y;";
};

// TEST(parser_test, digit) {
//     auto testparser = pc::parser(digit);
//     auto res = testparser.parse(tch::num).success().value();
//     auto res2 = testparser.parse(tch::str);
//     EXPECT_EQ('1', res);
//     EXPECT_FALSE(res2.is_success());
// }

TEST(parser_test, num) {
    auto testparser = pc::parser(num);
    auto res = testparser.parse(tch::num).success().value();
    auto res2 = testparser.parse(tch::str);
    EXPECT_EQ("123", res->to_string());
    EXPECT_FALSE(res2.is_success());

}

TEST(parser_test, symbol) {
    auto testparser = pc::parser(symbol);
    auto res = testparser.parse(tch::str).success().value();
    auto res2 = testparser.parse(tch::num);

    EXPECT_EQ("hoge", res->to_string());
    EXPECT_FALSE(res2.is_success());
}


TEST(parser_test, expr) {
    auto testparser = pc::parser(expr);
    auto res = testparser.parse(tch::exprnumnum);
    auto res2 = testparser.parse(tch::exprnumnumnum);

    // EXPECT_TRUE(res.is_success());
    EXPECT_EQ("(fcall + (1 2))",res.success().value()->to_string());
    // EXPECT_TRUE(res2.is_success());
    EXPECT_EQ("(fcall + ((fcall + (1 2)) 3))",res2.success().value()->to_string());
}
TEST(parser_test, list) {
    auto testparser = pc::parser(list);
    auto res = testparser.parse(tch::list);
    ListExpr test = *dynamic_cast<ListExpr*>(res.success().value().get());

    EXPECT_EQ("(symb hoge 123)",res.success().value()->to_string());
}
TEST(parser_test, fcall) {
    auto testparser = pc::parser(fcall);
    auto res = testparser.parse(tch::fcall);
    EXPECT_FALSE(!res.is_success());
    EXPECT_EQ("(fcall myfunc (these are args 1 2 3))",res.success().value()->to_string());
}
TEST(parser_test, assign) {
    auto testparser = pc::parser(assign);
    auto res = testparser.parse(tch::assign);
    EXPECT_FALSE(!res.is_success());
    EXPECT_EQ("(define test (fcall + (hoge 1)))",res.success().value()->to_string());
}
TEST(parser_test, lambda) {
    auto testparser = pc::parser(lambda);
    auto res = testparser.parse(tch::lambda);
    EXPECT_FALSE(!res.is_success());
    EXPECT_EQ("(lambda (x y z) (fcall + ((fcall + (x y)) z)))",res.success().value()->to_string());
}

TEST(parser_test, fdef) {
    auto testparser = pc::parser(fdef);
    auto res = testparser.parse(tch::fdef);
    EXPECT_FALSE(!res.is_success());
    EXPECT_EQ("(define test (lambda (x) (fcall + ((fcall + (hoge 1)) (fcall * (x y))))))",res.success().value()->to_string());
}

TEST(parser_test, statement) {
    auto testparser = pc::parser(statement);
    auto res = testparser.parse(tch::fdef);
    EXPECT_FALSE(!res.is_success());
    EXPECT_EQ("(define test (lambda (x) (fcall + ((fcall + (hoge 1)) (fcall * (x y))))))",res.success().value()->to_string());
    auto res2 = testparser.parse(tch::assign);
    EXPECT_FALSE(!res2.is_success());
    EXPECT_EQ("(define test (fcall + (hoge 1)))",res2.success().value()->to_string());
}
TEST(parser_test, top) {
    auto testparser = pc::parser(top);
    auto res = testparser.parse(tch::top);
    EXPECT_FALSE(!res.is_success());
    EXPECT_EQ("((define test (lambda (x) (fcall + ((fcall + (hoge 1)) (fcall * (x y)))))) (define test (fcall + (hoge 1))) (define test (lambda (x) (fcall + ((fcall + (hoge 1)) (fcall * (x y)))))))",res.success().value()->to_string());
}