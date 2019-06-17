#define MIMIUM_DEBUG

#include "gtest/gtest.h"
#include "driver.hpp"

#include "interpreter.hpp"

TEST(interpreter_test, assign) {
     mmmpsr::MimiumDriver driver;
     mimium::Interpreter interpreter;
     std::string teststr = "a = 1";
     driver.parsestring(teststr);
     mValue res = interpreter.loadAst(driver.getMainAst());
     EXPECT_EQ(mimium::Interpreter::to_string(res),"(assign a 1)") ;
}
TEST(interpreter_test, assign2) {
     mmmpsr::MimiumDriver driver;
     mimium::Interpreter interpreter;
     std::string teststr1 = "main = 1.245";
     driver.parsestring(teststr1);
     mValue res = interpreter.loadAst(driver.getMainAst());
     mValue main = interpreter.findVariable("main");
    double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,1.245);
}


TEST(interpreter_test, assignexpr) {
     mmmpsr::MimiumDriver driver;
     mimium::Interpreter interpreter2;
     std::string teststr2 = "main = 1+2.3/2.5*(2-1)+100";
     driver.parsestring(teststr2);
     mValue res = interpreter2.loadAst(driver.getMainAst());
     mValue main = interpreter2.findVariable("main");
    double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,101.92);
     } 

TEST(interpreter_test, assignfunction) {
     mmmpsr::MimiumDriver driver;
     mimium::Interpreter interpreter2;
     std::string teststr2 = "hoge(a,b) = a*b+1 \n main = hoge(7,5)";
     driver.parsestring(teststr2);
     mValue res = interpreter2.loadAst(driver.getMainAst());
     mValue main = interpreter2.findVariable("main");
    double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,36);
     } 

TEST(interpreter_test, assignfunction_block) {
     mmmpsr::MimiumDriver driver;
     mimium::Interpreter interpreter2;

     driver.parsefile("testfile_statements.mmm");
     mValue res = interpreter2.loadAst(driver.getMainAst());
     mValue main = interpreter2.findVariable("main");
    double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,8);
     } 
TEST(interpreter_test, localvar) {
     mmmpsr::MimiumDriver driver;
     mimium::Interpreter interpreter2;

     driver.parsefile("test_localvar.mmm");
     mValue res = interpreter2.loadAst(driver.getMainAst());
     mValue main = interpreter2.findVariable("main");
    double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,36);
     } 