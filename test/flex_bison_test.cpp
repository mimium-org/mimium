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
     EXPECT_EQ(ss.str(),"((assign b 3) (assign a 2) )");
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