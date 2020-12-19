#include "preprocessor/preprocessor.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
namespace fs = std::filesystem;

#ifndef TEST_ROOT_DIR
#define TEST_ROOT_DIR "\"\""
#endif

TEST(preprocessor, include) {
  fs::path root = TEST_ROOT_DIR;
  fs::path pptest_path = root / "preprocessor";
  fs::path includer = pptest_path / "includer.mmm";
  fs::path includee = pptest_path / "includee.mmm";
  fs::path answerpath = pptest_path / "answer.mmm";
  mimium::Preprocessor preprocessor(pptest_path);
  auto target = preprocessor.process(includer);
  auto answer = preprocessor.process(answerpath);
  EXPECT_TRUE(target.source==answer.source);
}