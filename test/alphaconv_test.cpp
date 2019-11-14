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
  mimium::Logger::current_report_level = mimium::Logger::INFO;
  runtime.init(visitor);
  runtime.loadSourceFile("test_localvar2.mmm");
  auto mainast = visitor->getResult();
  EXPECT_EQ(mainast->toString(),
            "((assign hoge1 (lambda (x2) ((assign localvar3 x2) (assign fuga4 (lambda (y5) ((assign localvar3 (+ localvar3 y5)) (return localvar3)))) (return (fuga4 (2.5)))))) (assign main6 (hoge1 (1))))");
}