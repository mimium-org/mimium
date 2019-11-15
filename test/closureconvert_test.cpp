#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
#include "helper_functions.hpp"
#include "alphaconvert_visitor.hpp"
#include "type_infer_visitor.hpp"
#include "knormalize_visitor.hpp"
#include "closure_convert.hpp"
#include "runtime.hpp"

static mimium::Runtime runtime;
static std::shared_ptr<mimium::AlphaConvertVisitor> alphavisitor;
static std::shared_ptr<mimium::TypeInferVisitor> typevisitor;
static std::shared_ptr<mimium::KNormalizeVisitor> knormvisitor;
static std::shared_ptr<mimium::ClosureConverter> closureconverter;

TEST(ClosureConvertTest, basic) {
  alphavisitor = std::make_shared<mimium::AlphaConvertVisitor>();
  typevisitor = std::make_shared<mimium::TypeInferVisitor>();
  knormvisitor = std::make_shared<mimium::KNormalizeVisitor>(typevisitor);
  closureconverter = std::make_shared<mimium::ClosureConverter>();


  runtime.setWorkingDirectory("/Users/tomoya/codes/mimium/build/test/");
  mimium::Logger::current_report_level = mimium::Logger::DEBUG;
  runtime.init(alphavisitor);
  runtime.loadSourceFile("test_closure.mmm");
  auto alphaast = alphavisitor->getResult();
  alphaast->accept(*typevisitor);
  alphaast->accept(*knormvisitor);
  auto mir = knormvisitor->getResult();
  auto converted = closureconverter->convert(mir);
  std::string ans = 
"main:\n"
"  countup4 = fun y5 fv{ localvar3 , x2 }\n"
"      countup4:\n"
"        localvar3 = localvar3+x2\n"
"        k1 = return localvar3\n"
"\n"
"  makecounter1 = fun x2\n"
"    makecounter1:\n"
"      localvar3 = 0.000000\n"
"      countup4$cls0 = makeclosure countup4 localvar3 , x2\n"
"      k2 = return countup4\n"
"\n"
"  k3 = 1.000000\n"
"  maincounter6 = app makecounter1 k3\n"
"  k4 = 1.000000\n"
"  main7 = appcls maincounter6 k4\n"
"  k5 = 1.000000\n"
"  main7 = appcls maincounter6 k5\n"
"  k6 = 1.000000\n"
"  main7 = appcls maincounter6 k6\n";
  EXPECT_EQ(ans,converted->toString());

};