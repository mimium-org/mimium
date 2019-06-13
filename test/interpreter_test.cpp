#define MIMIUM_DEBUG

#include "gtest/gtest.h"
#include "driver.hpp"

#include "interpreter.hpp"

TEST(interpreter_test, assign) {
     mmmpsr::MimiumDriver driver;
     mimium::Interpreter interpreter;
     std::string teststr = "a = 1";
     driver.parsestring(teststr);
     EXPECT_TRUE(interpreter.loadAst(driver.getMainAst()));
}
TEST(interpreter_test, assign2) {
     mmmpsr::MimiumDriver driver;
     mimium::Interpreter interpreter;
     std::string teststr1 = "main = 1.245";
     driver.parsestring(teststr1);
     interpreter.loadAst(driver.getMainAst());
    double res = interpreter.findVariable("main");
     EXPECT_EQ(res,1.245);
}


TEST(interpreter_test, assignexpr) {
     mmmpsr::MimiumDriver driver;
     mimium::Interpreter interpreter2;
     std::string teststr2 = "main = 1+2.3/2.5*(2-1)+100";
     driver.parsestring(teststr2);
     interpreter2.loadAst(driver.getMainAst());
    double res = interpreter2.findVariable("main");
     EXPECT_EQ(res,101.92);
     } 