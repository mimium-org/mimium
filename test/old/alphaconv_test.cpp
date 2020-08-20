#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
#include "basic/helper_functions.hpp"
#include "compiler/alphaconvert_visitor.hpp"
#include "runtime/runtime.hpp"

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
            "((assign hoge (lambda (x0) ((assign localvar1 x0) (assign fuga2 (lambda (y3) ((assign localvar1 (+ localvar1 y3)) (return localvar1)))) (return (fuga2 (2.5)))))) (assign main (hoge (1))))");
}