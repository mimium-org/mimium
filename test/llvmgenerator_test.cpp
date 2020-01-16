#include "compiler/alphaconvert_visitor.hpp"
#include "compiler/closure_convert.hpp"
#include "compiler/llvmgenerator.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
#include "basic/helper_functions.hpp"
#include "compiler/knormalize_visitor.hpp"
#include "runtime/runtime.hpp"
#include "compiler/type_infer_visitor.hpp"

static mimium::Runtime runtime;
static std::shared_ptr<mimium::KNormalizeVisitor> knormvisitor;
static std::shared_ptr<mimium::AlphaConvertVisitor> alphavisitor;
static std::shared_ptr<mimium::TypeInferVisitor> typevisitor;
static std::shared_ptr<mimium::ClosureConverter> closureconverter;
static std::shared_ptr<mimium::LLVMGenerator> llvmgenerator;

#define RESET_RUNTIME   runtime.clear();knormvisitor->init();alphavisitor->init();typevisitor->init();closureconverter->reset(); llvmgenerator->reset("test.ll");runtime.init(alphavisitor); 


TEST(LLVMGenerateTest, basic) {
    runtime.setWorkingDirectory("/Users/tomoya/codes/mimium/build/test/");
  mimium::Logger::current_report_level = mimium::Logger::DEBUG;

  alphavisitor = std::make_shared<mimium::AlphaConvertVisitor>();
  typevisitor = std::make_shared<mimium::TypeInferVisitor>();
  knormvisitor = std::make_shared<mimium::KNormalizeVisitor>(typevisitor);
  closureconverter = std::make_shared<mimium::ClosureConverter>(typevisitor->getEnv());
  llvmgenerator = std::make_shared<mimium::LLVMGenerator>("test.ll");
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
  RESET_RUNTIME
  runtime.init(alphavisitor);
  runtime.loadSource("mytest = print(100)");
  auto alphaast = alphavisitor->getResult();
  alphaast->accept(*typevisitor);
  alphaast->accept(*knormvisitor);
  auto mir = knormvisitor->getResult();
  auto converted = closureconverter->convert(mir);
  llvmgenerator->generateCode(converted);
  std::string test = "";
  llvm::raw_string_ostream ss(test);
  llvmgenerator->outputToStream(llvm::outs());

  std::string ans ="";
  EXPECT_EQ(ans, ss.str());
};