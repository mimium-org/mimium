#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
#include "helper_functions.hpp"
#include "knormalize_visitor.hpp"
#include "runtime.hpp"

static mimium::Runtime runtime;
static std::shared_ptr<mimium::KNormalizeVisitor> knormvisitor;
int mimium::MIRblock::indent_level = 0;  // actual instance of indent level

TEST(knormalizetest, basic) {
  knormvisitor = std::make_shared<mimium::KNormalizeVisitor>();
  runtime.setWorkingDirectory("/Users/tomoya/codes/mimium/build/test/");
  mimium::Logger::current_report_level = mimium::Logger::DEBUG;
  runtime.init(knormvisitor);
  std::string teststr = "a = 1+2+3";
  runtime.loadSource(teststr);
  auto mainast = knormvisitor->getResult();
  std::string ans =
      "main:\n"
      "  k2 = 1.000000\n"
      "  k3 = 2.000000\n"
      "  k1 = k2+k3\n"
      "  k4 = 3.000000\n"
      "  a = k1+k4\n";
  EXPECT_EQ(mainast->toString(), ans);
}
TEST(knormalizetest, multiline) {
  runtime.clear();
  knormvisitor->init();
  runtime.init(knormvisitor);
  std::string teststr = "a = 1+2+3 \n b = 4-150*20";
  runtime.loadSource(teststr);
  auto mainast = knormvisitor->getResult();
  EXPECT_EQ(mainast->toString(),
            "main:\n"
            "  k2 = 1.000000\n"
            "  k3 = 2.000000\n"
            "  k1 = k2+k3\n"
            "  k4 = 3.000000\n"
            "  a = k1+k4\n"
            "  k5 = 4.000000\n"
            "  k7 = 150.000000\n"
            "  k8 = 20.000000\n"
            "  k6 = k7*k8\n"
            "  b = k5-k6\n");
}
TEST(knormalizetest, if_nested) {
    runtime.clear();
  knormvisitor->init();
  runtime.init(knormvisitor);
  runtime.loadSourceFile("test_if_nested.mmm");
  auto mainast = knormvisitor->getResult();
  EXPECT_EQ(mainast->toString(),
            "main:\n"
            "  k2 = 1.000000\n"
            "  k3 = 2.000000\n"
            "  k1 = k2+k3\n"
            "  k4 = 3.000000\n"
            "  a = k1+k4\n"
            "  k5 = 4.000000\n"
            "  k7 = 150.000000\n"
            "  k8 = 20.000000\n"
            "  k6 = k7*k8\n"
            "  b = k5-k6\n");
}