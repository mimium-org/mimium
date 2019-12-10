#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
#include "helper_functions.hpp"
#include "alphaconvert_visitor.hpp"
#include "knormalize_visitor.hpp"
#include "runtime.hpp"

static mimium::Runtime runtime;
static std::shared_ptr<mimium::KNormalizeVisitor> knormvisitor;
static std::shared_ptr<mimium::AlphaConvertVisitor> alphaconv;
static std::shared_ptr<mimium::TypeInferVisitor> typeinfer;


#define RESET_RUNTIME   runtime.clear();knormvisitor->init();alphaconv->init();typeinfer->init();runtime.init(alphaconv); 


TEST(knormalizetest, basic) {
  mimium::Logger::current_report_level = mimium::Logger::INFO;
  alphaconv = std::make_shared<mimium::AlphaConvertVisitor>();
  typeinfer = std::make_shared<mimium::TypeInferVisitor>();
  knormvisitor = std::make_shared<mimium::KNormalizeVisitor>(typeinfer);
  runtime.setWorkingDirectory("/Users/tomoya/codes/mimium/build/test/");
  runtime.init(alphaconv);  
  std::string teststr = "a = 1+2+3";
  runtime.loadSource(teststr);
  auto alphaast = alphaconv->getResult();
  alphaast->accept(*typeinfer);
  alphaast->accept(*knormvisitor);
  auto mainmir = knormvisitor->getResult();
  std::string ans =
      "main:\n"
      "  k2 = 1.000000\n"
      "  k3 = 2.000000\n"
      "  k1 = k2+k3\n"
      "  k4 = 3.000000\n"
      "  a = k1+k4\n";
  EXPECT_EQ(mainmir->toString(), ans);
}
TEST(knormalizetest, multiline) {
  RESET_RUNTIME
  std::string teststr = "a = 1+2+3 \n b = 4-150*20";
  runtime.loadSource(teststr);
  auto alphaast = alphaconv->getResult();
  alphaast->accept(*typeinfer);
  alphaast->accept(*knormvisitor);
  auto mainmir = knormvisitor->getResult();
  EXPECT_EQ(mainmir->toString(),
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
  RESET_RUNTIME
  runtime.loadSourceFile("test_localvar.mmm");
  auto alphaast = alphaconv->getResult();
  alphaast->accept(*typeinfer);
  alphaast->accept(*knormvisitor);
  auto mainmir = knormvisitor->getResult();
  EXPECT_EQ(mainmir->toString(),
            "main:\n"
            "  hoge = fun x0 , y1\n"
            "    hoge:\n"
            "      localvar2 = 2.000000\n"
            "      k2 = x0*y1\n"
            "      k1 = k2+localvar2\n"
            "      k3 = return k1\n"
            "\n"
            "  k4 = 5.000000\n"
            "  k5 = 7.000000\n"
            "  main = appcls hoge k4 , k5\n");
}
TEST(knormalizetest, if_nested) {
  RESET_RUNTIME
  runtime.loadSourceFile("test_if_nested.mmm");
  auto alphaast = alphaconv->getResult();
  alphaast->accept(*typeinfer);
  alphaast->accept(*knormvisitor);
  auto mainmir = knormvisitor->getResult();
  EXPECT_EQ(mainmir->toString(),
            "main:\n"
            "  test = fun x0 , y1 , z2\n"
            "    test:\n"
            "      k1 = if x0\n"
            "    k1$then:\n"
            "      res3 = 0.000000\n"
            "    k1$else:\n"
            "      k2 = if y1\n"
            "      k2$then:\n"
            "        res3 = 100.000000\n"
            "      k2$else:\n"
            "\n"
            "\n"
            "      res3 = return res3\n"
            "\n"
            "  k3 = 1.000000\n"
            "  k4 = 23.000000\n"
            "  k5 = 244.000000\n"
            "  zero = appcls test k3 , k4 , k5\n"
            "  k6 = 0.000000\n"
            "  k7 = 200.000000\n"
            "  k8 = 400.000000\n"
            "  hand = appcls test k6 , k7 , k8\n"
            "  k9 = 0.000000\n"
            "  k10 = 0.000000\n"
            "  k11 = 500.000000\n"
            "  fivehand = appcls test k9 , k10 , k11\n"

  );
}
TEST(knormalizetest, closure) {
  RESET_RUNTIME
  runtime.loadSourceFile("test_closure.mmm");
  auto alphaast = alphaconv->getResult();
  alphaast->accept(*typeinfer);
  alphaast->accept(*knormvisitor);
  auto mainmir = knormvisitor->getResult();
  // std::cout << mainmir->toString() << std::endl;
  EXPECT_EQ(mainmir->toString(),
            "main:\n"
            "  makecounter = fun x0\n"
            "    makecounter:\n"
            "      localvar1 = 0.000000\n"
            "      countup2 = fun y3\n"
            "      countup2:\n"
            "        localvar1 = localvar1+x0\n"
            "        k1 = return localvar1\n"
            "\n"
            "      k2 = return countup2\n"
            "\n"
            "  k3 = 1.000000\n"
            "  maincounter = appcls makecounter k3\n"
            "  k4 = 1.000000\n"
            "  main = appcls maincounter k4\n"
            "  k5 = 1.000000\n"
            "  main = appcls maincounter k5\n"
            "  k6 = 1.000000\n"
            "  main = appcls maincounter k6\n");
}