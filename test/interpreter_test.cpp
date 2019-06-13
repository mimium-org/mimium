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