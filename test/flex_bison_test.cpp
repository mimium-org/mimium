#define MIMIUM_DEBUG

#include "gtest/gtest.h"
#include "driver.hpp"


TEST(bison_parser_test, assign) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "a = 1";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign a 1) )");
}
TEST(bison_parser_test, assignfpoint) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "a = 1.245";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign a 1.245) )");
}

TEST(bison_parser_test, expr) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "a = 1+2+3*2/2";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign a (+ (+ 1 2) (/ (* 3 2) 2))) )");
}

TEST(bison_parser_test, parensis) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "a=(2+3)/5";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign a (/ (+ 2 3) 5)) )");
}

TEST(bison_parser_test, lines) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "a=2 \n b=3";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign a 2) (assign b 3) )");
}

TEST(bison_parser_test, fdef) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "myfunc(a,b) = 1+2";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign myfunc (lambda ((a b ))(+ 1 2))) )");
}
TEST(bison_parser_test, lambda) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "myfunc = (a,b)->{1+2}";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign myfunc (lambda ((a b ))(+ 1 2))) )");
}


TEST(bison_parser_test, time) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "a = (3+2)@50";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign a (+ 3 2)@50) )");
}


TEST(bison_parser_test, fileread) {
     mmmpsr::MimiumDriver driver;
     driver.parsefile("testfile1.mmm");
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign a (+ 2 3)@128) )");
}

TEST(bison_parser_test, func_block) {
          mmmpsr::MimiumDriver driver;
     driver.parsefile("testfile_blockfun.mmm");
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign hoge (lambda ((x y ))(( return 1) ))) )");
}

TEST(bison_parser_test, statements) {
          mmmpsr::MimiumDriver driver;
     driver.parsefile("testfile_statements.mmm");
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign hoge (lambda ((x y ))(( return (+ x y)) ))) (assign main (hoge (3 5 ))) )");
}
