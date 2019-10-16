#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
#include "helper_functions.hpp"
#include "alphaconvert_visitor.hpp"
#include "runtime.hpp"

static mimium::Runtime runtime;
static std::shared_ptr<mimium::AlphaConvertVisitor> visitor;

TEST(alphaconverttest, basic) {
  visitor = std::make_shared<mimium::AlphaConvertVisitor>();
  runtime.setWorkingDirectory("/Users/tomoya/codes/mimium/build/test/");
  mimium::Logger::current_report_level = mimium::Logger::DEBUG;
  runtime.init(visitor);
  runtime.loadSourceFile("test_localvar2.mmm");
  auto mainast = visitor->getResult();
  EXPECT_EQ(mainast->toString(),
            "((assign var1 (lambda (var2) ((assign var3 var2) (assign var4 (lambda (var5) ((assign var6 (+ var6 var5)) (return var6)))) (return (var4 (2.5)))))) (assign var7 (var1 (1))))");
}