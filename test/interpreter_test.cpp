#define MIMIUM_DEBUG
#include <cmath>
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
#include "helper_functions.hpp"
#include "interpreter_visitor.hpp"
static mmmpsr::MimiumDriver driver;
static auto interpreter =  std::make_shared<mimium::InterpreterVisitor>();

// static auto& runtime = interpreter->getRuntime();
TEST(interpreter_test, assign) {
     mimium::Logger::current_report_level = mimium::Logger::INFO;
     interpreter->init();
     interpreter->getRuntime()->loadSource("a = 1");
     EXPECT_EQ(mimium::Runtime::to_string(interpreter->getRuntime()->getMainAst()),"(assign a 1)") ;
}
TEST(interpreter_test, assign2) {
     interpreter->getRuntime()->clear();
     interpreter->getRuntime()->loadSource("main = 1.245");
     mValue main = interpreter->getRuntime()->findVariable("main");
     double resv = std::get<double>(main);
     EXPECT_EQ(resv,1.245);
}
TEST(interpreter_test, assignexpr) {
     interpreter->getRuntime()->clear();
     interpreter->getRuntime()->loadSource("main = 1+2.3/2.5*(2-1)+100");
     mValue main = interpreter->getRuntime()->findVariable("main");
     double resv = std::get<double>(main);
     EXPECT_EQ(resv,101.92);
     } 

TEST(interpreter_test, assignfunction) {
     interpreter->getRuntime()->clear();
     interpreter->getRuntime()->loadSource("fn hoge(a,b){return a*b+1} \n main = hoge(7,5)");   
     mValue main = interpreter->getRuntime()->findVariable("main");
     double resv = std::get<double>(main);
     EXPECT_EQ(resv,36);
} 

TEST(interpreter_test, assignfunction_block) {
     interpreter->getRuntime()->setWorkingDirectory("/Users/tomoya/codes/mimium/build/test/");
     interpreter->getRuntime()->clear();
     interpreter->getRuntime()->loadSourceFile("testfile_statements.mmm");
     mValue main = interpreter->getRuntime()->findVariable("main");
     double resv = std::get<double>(main);
     EXPECT_EQ(resv,8);
     } 
TEST(interpreter_test, localvar) {
     interpreter->getRuntime()->clear();
     interpreter->getRuntime()->loadSourceFile("test_localvar.mmm");
     mValue main = interpreter->getRuntime()->findVariable("main");
     double resv = std::get<double>(main);
     EXPECT_EQ(resv,37);
     } 
TEST(interpreter_test, localvar_nestfun) {
     interpreter->getRuntime()->clear();
     interpreter->getRuntime()->loadSourceFile("test_localvar2.mmm");
     mValue main = interpreter->getRuntime()->findVariable("main");
     double resv = std::get<double>(main);
     EXPECT_EQ(resv,3.5);
}
TEST(interpreter_test, closure_countup) {
     interpreter->getRuntime()->clear();
     interpreter->getRuntime()->loadSourceFile("test_closure.mmm");
     mValue main = interpreter->getRuntime()->findVariable("main");
     double resv = std::get<double>(main);
     EXPECT_EQ(resv,3);
}
TEST(interpreter_test, builtin_print) {
     interpreter->getRuntime()->clear();
     interpreter->getRuntime()->loadSource("main=println(1)");   
     mValue main = interpreter->getRuntime()->findVariable("main");
     double resv = std::get<double>(main);
     EXPECT_EQ(resv,0);
} 

TEST(interpreter_test, ifstatement) {
     interpreter->getRuntime()->clear();
     interpreter->getRuntime()->loadSourceFile("test_if.mmm");
     mValue main = interpreter->getRuntime()->findVariable("true");
     double resv = std::get<double>(main);
     EXPECT_EQ(resv,5);
     mValue main2 = interpreter->getRuntime()->findVariable("false");
     double resv2 = std::get<double>(main2);
     EXPECT_EQ(resv2,100);
} 
TEST(interpreter_test, ifstatement2) {
     interpreter->getRuntime()->clear();
     interpreter->getRuntime()->loadSourceFile("test_if_nested.mmm");
     mValue main = interpreter->getRuntime()->findVariable("zero");
     double resv = std::get<double>(main);
     EXPECT_EQ(resv,0);
     mValue main2 = interpreter->getRuntime()->findVariable("hand");
     double resv2 = std::get<double>(main2);
     EXPECT_EQ(resv2,100);
     mValue main3 = interpreter->getRuntime()->findVariable("fivehand");
     double resv3 = std::get<double>(main3);
     EXPECT_EQ(resv3,500);
} 
TEST(interpreter_test, array) {
     interpreter->getRuntime()->clear();
     interpreter->getRuntime()->loadSource("main = [1,2,3,4,5]");   
     mValue main = interpreter->getRuntime()->findVariable("main");
     EXPECT_EQ("[1,2,3,4,5]", mimium::Runtime::to_string(main) );
} 
TEST(interpreter_test, arrayaccess) {
     interpreter->getRuntime()->clear();
     interpreter->getRuntime()->loadSource("arr = [1,2,3,4,5]\n\
                             main = arr[2]");   
     mValue main = interpreter->getRuntime()->findVariable("main");
     EXPECT_EQ(3, std::get<double>(main) );
} 
TEST(interpreter_test, forloop) {
     interpreter->getRuntime()->clear();
     interpreter->getRuntime()->loadSource("arr = [1,2,3,4,5]\n\
                         main=0\n\
                         for i in arr {\n\
                              main = main+i  \n\
                         }");
     mValue main = interpreter->getRuntime()->findVariable("main");
     EXPECT_EQ(15, std::get<double>(main));
} 


TEST(interpreter_test, factorial) {
     interpreter->getRuntime()->clear();
     interpreter->getRuntime()->loadSourceFile("factorial.mmm");
     mValue main = interpreter->getRuntime()->findVariable("main");
     EXPECT_EQ(120, std::get<double>(main));
}

TEST(interpreter_test, fibonacchi) {
     interpreter->getRuntime()->clear();
     interpreter->getRuntime()->loadSourceFile("fibonacchi.mmm");
     mValue main = interpreter->getRuntime()->findVariable("main");
     EXPECT_EQ(610, std::get<double>(main));
}
TEST(interpreter_test, include) {
     interpreter->getRuntime()->clear();
     interpreter->getRuntime()->loadSourceFile("test_include.mmm");
     mValue main = interpreter->getRuntime()->findVariable("main");
     EXPECT_EQ(25+128, std::get<double>(main));
}
TEST(interpreter_test, mathtest) {
     interpreter->getRuntime()->clear();
     interpreter->getRuntime()->loadSource("main=sin(3.14)");
     mValue main = interpreter->getRuntime()->findVariable("main");
     EXPECT_EQ(std::sin(3.14), std::get<double>(main));
}

