#include "alphaconvert_visitor.hpp"
#include "closure_convert.hpp"
#include "llvmgenerator.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
#include "helper_functions.hpp"
#include "knormalize_visitor.hpp"
#include "runtime.hpp"
#include "type_infer_visitor.hpp"

static mimium::Runtime runtime;

TEST(LLVMGenerateTest, basic) {
    runtime.setWorkingDirectory("/Users/tomoya/codes/mimium/build/test/");
  mimium::Logger::current_report_level = mimium::Logger::DEBUG;

  auto alphavisitor = std::make_shared<mimium::AlphaConvertVisitor>();
  auto typevisitor = std::make_shared<mimium::TypeInferVisitor>();
  auto knormvisitor = std::make_shared<mimium::KNormalizeVisitor>(typevisitor);
  auto closureconverter = std::make_shared<mimium::ClosureConverter>(typevisitor->getEnv());
  auto llvmgenerator = std::make_shared<mimium::LLVMGenerator>("test.ll");
  runtime.init(alphavisitor);
  runtime.loadSourceFile("test_closure2.mmm");
  auto alphaast = alphavisitor->getResult();
  alphaast->accept(*typevisitor);
  alphaast->accept(*knormvisitor);
  auto mir = knormvisitor->getResult();
  auto converted = closureconverter->convert(mir);
  std::cout << converted->toString() <<std::endl;

  llvmgenerator->generateCode(converted);
  std::string test = "";
  llvm::raw_string_ostream ss(test);
  llvmgenerator->outputToStream(llvm::outs());

  std::string ans ="";
  EXPECT_EQ(ans, ss.str());
};

TEST(LLVMGenerateTest, builtin) {
  auto alphavisitor = std::make_shared<mimium::AlphaConvertVisitor>();
  auto typevisitor = std::make_shared<mimium::TypeInferVisitor>();
  auto knormvisitor = std::make_shared<mimium::KNormalizeVisitor>(typevisitor);
  auto closureconverter = std::make_shared<mimium::ClosureConverter>(typevisitor->getEnv());
  auto llvmgenerator = std::make_shared<mimium::LLVMGenerator>("test.ll");
  runtime.init(alphavisitor);
  runtime.loadSource("main = print(100)");
  auto alphaast = alphavisitor->getResult();
  alphaast->accept(*typevisitor);
  alphaast->accept(*knormvisitor);
  auto mir = knormvisitor->getResult();
  auto converted = closureconverter->convert(mir);
  llvmgenerator->generateCode(converted);
  std::string test = "";
  llvm::raw_string_ostream ss(test);
  llvmgenerator->outputToStream(ss);

  std::string ans ="";
  EXPECT_EQ(ans, ss.str());
};