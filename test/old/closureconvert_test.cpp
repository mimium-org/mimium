#include "compiler/alphaconvert_visitor.hpp"
#include "compiler/closure_convert.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
#include "basic/helper_functions.hpp"
#include "compiler/knormalize_visitor.hpp"
#include "runtime/runtime.hpp"
#include "compiler/type_infer_visitor.hpp"

static mimium::Runtime runtime;
static std::shared_ptr<mimium::AlphaConvertVisitor> alphavisitor;
static std::shared_ptr<mimium::TypeInferVisitor> typevisitor;
static std::shared_ptr<mimium::KNormalizeVisitor> knormvisitor;
static std::shared_ptr<mimium::ClosureConverter> closureconverter;

TEST(ClosureConvertTest, basic) {
  alphavisitor = std::make_shared<mimium::AlphaConvertVisitor>();
  typevisitor = std::make_shared<mimium::TypeInferVisitor>();
  knormvisitor = std::make_shared<mimium::KNormalizeVisitor>(typevisitor);
  closureconverter =
      std::make_shared<mimium::ClosureConverter>(typevisitor->getEnv());

  runtime.setWorkingDirectory("/Users/tomoya/codes/mimium/build/test/");
  mimium::Logger::current_report_level = mimium::Logger::DEBUG;
  runtime.init(alphavisitor);
  runtime.loadSourceFile("test_closure.mmm");
  auto alphaast = alphavisitor->getResult();
  alphaast->accept(*typevisitor);
  alphaast->accept(*knormvisitor);
  auto mir = knormvisitor->getResult();
  auto converted = closureconverter->convert(mir);
  // std::cout << converted->toString()<<std::endl;
  std::string ans =
      "main:\n"
      "  countup2 = fun y3 fv{ localvar1 , x0 }\n"
      "      countup2:\n"
      "        localvar1 = fv_localvar1+fv_x0\n"
      "        k1 = return fv_localvar1\n"
      "\n"
      "  makecounter = fun x0\n"
      "    makecounter:\n"
      "      localvar1 = 0.000000\n"
      "      countup2_cls = makeclosure countup2 localvar1 , x0\n"
      "      k2 = return countup2\n"
      "\n"
      "  k3 = 1.000000\n"
      "  maincounter = app makecounter k3\n"
      "  k4 = 1.000000\n"
      "  main = appcls maincounter k4\n"
      "  k5 = 1.000000\n"
      "  main = appcls maincounter k5\n"
      "  k6 = 1.000000\n"
      "  main = appcls maincounter k6\n";
  EXPECT_EQ(ans, converted->toString());
};

TEST(ClosureConvertTest, localvar) {
  alphavisitor.reset();
  alphavisitor = std::make_shared<mimium::AlphaConvertVisitor>();
  typevisitor.reset();
  typevisitor = std::make_shared<mimium::TypeInferVisitor>();
  knormvisitor.reset();
  knormvisitor = std::make_shared<mimium::KNormalizeVisitor>(typevisitor);
  closureconverter.reset();
  closureconverter =
      std::make_shared<mimium::ClosureConverter>(typevisitor->getEnv());
  runtime.clear();
  runtime.init(alphavisitor);
  runtime.loadSourceFile("test_localvar.mmm");
  auto alphaast = alphavisitor->getResult();
  alphaast->accept(*typevisitor);
  alphaast->accept(*knormvisitor);
  auto mir = knormvisitor->getResult();
  // std::cout << mir->toString()<<std::endl;
  auto converted = closureconverter->convert(mir);
  std::string ans =
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
      "  main = app hoge k4 , k5\n";

  EXPECT_EQ(ans, converted->toString());
};