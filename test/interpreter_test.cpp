#define MIMIUM_DEBUG

#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"

#include "driver.hpp"

#include "interpreter.hpp"
static mmmpsr::MimiumDriver driver;
static mimium::Interpreter interpreter;
TEST(interpreter_test, assign) {
     std::string teststr = "a = 1";
     driver.parsestring(teststr);
     mValue res = interpreter.loadAst(driver.getMainAst());
     EXPECT_EQ(mimium::Interpreter::to_string(res),"(assign a 1)") ;
}
TEST(interpreter_test, assign2) {
     std::string teststr1 = "main = 1.245";
     driver.parsestring(teststr1);
     mValue res = interpreter.loadAst(driver.getMainAst());
     mValue main = interpreter.findVariable("main");
    double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,1.245);
}
TEST(interpreter_test, assignexpr) {
     std::string teststr2 = "main = 1+2.3/2.5*(2-1)+100";
     driver.parsestring(teststr2);
     mValue res = interpreter.loadAst(driver.getMainAst());
     mValue main = interpreter.findVariable("main");
    double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,101.92);
     } 

TEST(interpreter_test, assignfunction) {
     std::string teststr2 = "fn hoge(a,b){return a*b+1} \n main = hoge(7,5)";
     driver.parsestring(teststr2);
     mValue res = interpreter.loadAst(driver.getMainAst());
     mValue main = interpreter.findVariable("main");
    double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,36);
     } 

TEST(interpreter_test, assignfunction_block) {
     driver.parsefile("testfile_statements.mmm");
     mValue res = interpreter.loadAst(driver.getMainAst());
     mValue main = interpreter.findVariable("main");
    double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,8);
     } 
TEST(interpreter_test, localvar) {
     driver.parsefile("test_localvar.mmm");
     mValue res = interpreter.loadAst(driver.getMainAst());
     mValue main = interpreter.findVariable("main");
    double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,37);
     } 
TEST(interpreter_test, localvar_nestfun) {
     driver.parsefile("test_localvar2.mmm");
     mValue res = interpreter.loadAst(driver.getMainAst());
     mValue main = interpreter.findVariable("main");
    double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,3.5);
}
TEST(interpreter_test, closure_countup) {
     driver.parsefile("test_closure.mmm");
     mValue res = interpreter.loadAst(driver.getMainAst());
     mValue main = interpreter.findVariable("main");
    double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,3);
}
TEST(interpreter_test, builtin_print) {
     std::string teststr1 = "main = print(1)";
     driver.parsestring(teststr1);
     mValue res = interpreter.loadAst(driver.getMainAst());
     mValue main = interpreter.findVariable("main");
    double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,0);
} 

TEST(interpreter_test, ifstatement) {
     driver.parsefile("test_if.mmm");
     mValue res = interpreter.loadAst(driver.getMainAst());
     mValue main = interpreter.findVariable("true");
    double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,5);
     mValue main2 = interpreter.findVariable("false");
    double resv2 = mimium::Interpreter::get_as_double(main2);
     EXPECT_EQ(resv2,100);
} 
TEST(interpreter_test, ifstatement2) {
     driver.parsefile("test_if_nested.mmm");
     mValue res = interpreter.loadAst(driver.getMainAst());
     mValue main = interpreter.findVariable("zero");
    double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,0);
     mValue main2 = interpreter.findVariable("hand");
    double resv2 = mimium::Interpreter::get_as_double(main2);
     EXPECT_EQ(resv2,100);
     mValue main3 = interpreter.findVariable("fivehand");
    double resv3 = mimium::Interpreter::get_as_double(main3);
     EXPECT_EQ(resv3,500);
} 
TEST(interpreter_test, array) {
     std::string teststr1 = "main = [1,2,3,4,5]";
     driver.parsestring(teststr1);
     mValue res = interpreter.loadAst(driver.getMainAst());
     mValue main = interpreter.findVariable("main");
    EXPECT_EQ("[1,2,3,4,5]", mimium::Interpreter::to_string(main) );
} 
TEST(interpreter_test, arrayaccess) {
     std::string teststr1 = "arr = [1,2,3,4,5]\n\
                             main = arr[2]";
     driver.parsestring(teststr1);
     mValue res = interpreter.loadAst(driver.getMainAst());
     mValue main = interpreter.findVariable("main");
    EXPECT_EQ(3, mimium::Interpreter::get_as_double(main));
} 

TEST(interpreter_test, factorial) {
     driver.parsefile("factorial.mmm");
     mValue res = interpreter.loadAst(driver.getMainAst());
     mValue main = interpreter.findVariable("main");
    EXPECT_EQ(120, mimium::Interpreter::get_as_double(main));
}

TEST(interpreter_test, fibonacchi) {
     mimium::Interpreter intp2;
     driver.parsefile("fibonacchi.mmm");
     mValue res = intp2.loadAst(driver.getMainAst());
     mValue main = intp2.findVariable("main");
    EXPECT_EQ(610, mimium::Interpreter::get_as_double(main));
}
