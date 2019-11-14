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
  std::string ans = "";
  EXPECT_EQ(ans,converted->toString());

};