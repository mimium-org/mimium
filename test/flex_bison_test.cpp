#include "gtest/gtest.h"
#include "driver.hpp"

TEST(bison_parser_test, expr) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "1+2+3*2/2";
     driver.parse(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((+ (+ 1 2) (/ (* 3 2) 2)) )");
}
