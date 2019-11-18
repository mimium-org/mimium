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
static std::shared_ptr<mimium::AlphaConvertVisitor> alphavisitor;
static std::shared_ptr<mimium::TypeInferVisitor> typevisitor;
static std::shared_ptr<mimium::KNormalizeVisitor> knormvisitor;
static std::shared_ptr<mimium::ClosureConverter> closureconverter;
static std::shared_ptr<mimium::LLVMGenerator> llvmgenerator;

TEST(LLVMGenerateTest, basic) {
  alphavisitor = std::make_shared<mimium::AlphaConvertVisitor>();
  typevisitor = std::make_shared<mimium::TypeInferVisitor>();
  knormvisitor = std::make_shared<mimium::KNormalizeVisitor>(typevisitor);
  closureconverter = std::make_shared<mimium::ClosureConverter>();
  llvmgenerator = std::make_shared<mimium::LLVMGenerator>("test.ll");


  runtime.setWorkingDirectory("/Users/tomoya/codes/mimium/build/test/");
  mimium::Logger::current_report_level = mimium::Logger::DEBUG;
  runtime.init(alphavisitor);
  runtime.loadSourceFile("test_emit_llvm.mmm");
  auto alphaast = alphavisitor->getResult();
  alphaast->accept(*typevisitor);
  alphaast->accept(*knormvisitor);
  auto mir = knormvisitor->getResult();
      std::cout << mir->toString() << std::endl;

  auto converted = closureconverter->convert(mir);
    std::cout << converted->toString() << std::endl;
  llvmgenerator->generateCode(converted);
  std::string test = "";
  llvm::raw_string_ostream ss(test);
  llvmgenerator->outputToStream(ss);

  std::string ans ="";
  EXPECT_EQ(ans, ss.str());
};