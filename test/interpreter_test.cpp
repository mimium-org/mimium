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
    mValue res = interpreter.findVariable("main");
    double resv = mimium::Interpreter::get_as_double(res);
     EXPECT_EQ(resv,1.245);
}


TEST(interpreter_test, assignexpr) {
     mmmpsr::MimiumDriver driver;
     mimium::Interpreter interpreter2;
     std::string teststr2 = "main = 1+2.3/2.5*(2-1)+100";
     driver.parsestring(teststr2);
     interpreter2.loadAst(driver.getMainAst());
    mValue res = interpreter2.findVariable("main");
     double resv = mimium::Interpreter::get_as_double(res);

     EXPECT_EQ(resv,101.92);
     } 

TEST(interpreter_test, assignfunction) {
     mmmpsr::MimiumDriver driver;
     mimium::Interpreter interpreter2;
     std::string teststr2 = "fun(a,b) = a*b+1 \n main = fun(7,5)";
     driver.parsestring(teststr2);
     interpreter2.loadAst(driver.getMainAst());
    mValue res = interpreter2.findVariable("main");
     double resv = mimium::Interpreter::get_as_double(res);

     EXPECT_EQ(resv,36);
     } 