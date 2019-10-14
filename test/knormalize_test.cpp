#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
#include "helper_functions.hpp"
#include "knormalize_visitor.hpp"
#include "runtime.hpp"

static mimium::Runtime runtime;
static std::shared_ptr<mimium::KNormalizeVisitor> knormvisitor;

TEST(knormalizetest, basic) {
  knormvisitor = std::make_shared<mimium::KNormalizeVisitor>();
  runtime.setWorkingDirectory("/Users/tomoya/codes/mimium/build/test/");
  mimium::Logger::current_report_level = mimium::Logger::DEBUG;
  runtime.init(knormvisitor);
  std::string teststr = "a = 1+2+3";
  runtime.loadSource(teststr);
  auto mainast = knormvisitor->getResult();
  EXPECT_EQ(mainast->toString(),
            "((assign tmp1 (+ 1 2)) (assign a (+ tmp1 3)))");
}
TEST(knormalizetest, multiline) {
    runtime.clear();
    knormvisitor->init();
  runtime.init(knormvisitor);
  std::string teststr = "a = 1+2+3 \n b = 4-150*20";
  runtime.loadSource(teststr);
  auto mainast = knormvisitor->getResult();
  EXPECT_EQ(mainast->toString(),
            "((assign tmp1 (+ 1 2)) (assign a (+ tmp1 3)) (assign tmp2 (* 150 20)) (assign b (- 4 tmp2)))");
}