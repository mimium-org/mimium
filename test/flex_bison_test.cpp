#include "gtest/gtest.h"
#include "driver.hpp"

TEST(bison_parser_test, expr) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "1+2+3";
     driver.parse(teststr);
     driver.print(std::cout);
}