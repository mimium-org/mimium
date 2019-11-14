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
  mimium::Logger::current_report_level = mimium::Logger::INFO;
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
TEST(knormalizetest, localvar) {
  runtime.clear();
  knormvisitor->init();
  runtime.init(knormvisitor);
  runtime.loadSourceFile("test_localvar.mmm");
  auto mainast = knormvisitor->getResult();
  EXPECT_EQ(mainast->toString(),
            "main:\n"
            "  hoge = fun x , y\n"
            "    hoge:\n"
            "      localvar = 2.000000\n"
            "      k2 = x*y\n"
            "      k1 = k2+localvar\n"
            "      k3 = return k1\n"
            "\n"
            "  k4 = 5.000000\n"
            "  k5 = 7.000000\n"
            "  main = app hoge k4 , k5\n");
}
TEST(knormalizetest, if_nested) {
  runtime.clear();
  knormvisitor->init();
  runtime.init(knormvisitor);
  runtime.loadSourceFile("test_if_nested.mmm");
  auto mainast = knormvisitor->getResult();
  EXPECT_EQ(mainast->toString(),
            "main:\n"
            "  test = fun x , y , z\n"
            "    test:\n"
            "      k1 = if x\n"
            "    k1$then:\n"
            "      res = 0.000000\n"
            "    k1$else:\n"
            "      k2 = if y\n"
            "      k2$then:\n"
            "        res = 100.000000\n"
            "      k2$else:\n"
            "\n"
            "\n"
            "      res = return res\n"
            "\n"
            "  k3 = 1.000000\n"
            "  k4 = 23.000000\n"
            "  k5 = 244.000000\n"
            "  zero = app test k3 , k4 , k5\n"
            "  k6 = 0.000000\n"
            "  k7 = 200.000000\n"
            "  k8 = 400.000000\n"
            "  hand = app test k6 , k7 , k8\n"
            "  k9 = 0.000000\n"
            "  k10 = 0.000000\n"
            "  k11 = 500.000000\n"
            "  fivehand = app test k9 , k10 , k11\n"

  );
}
TEST(knormalizetest, closure) {
  runtime.clear();
  knormvisitor->init();
  runtime.init(knormvisitor);
  runtime.loadSourceFile("test_closure.mmm");
  auto mainast = knormvisitor->getResult();
  EXPECT_EQ(mainast->toString(),
            "main:\n"
            "  makecounter = fun x\n"
            "    makecounter:\n"
            "      localvar = 0.000000\n"
            "      countup = fun y\n"
            "      countup:\n"
            "        localvar = localvar+x\n"
            "        k1 = return localvar\n"
            "\n"
            "      k2 = return countup\n"
            "\n"
            "  k3 = 1.000000\n"
            "  maincounter = app makecounter k3\n"
            "  k4 = 1.000000\n"
            "  main = app maincounter k4\n"
            "  k5 = 1.000000\n"
            "  main = app maincounter k5\n"
            "  k6 = 1.000000\n"
            "  main = app maincounter k6\n");
}