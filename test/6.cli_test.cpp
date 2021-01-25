
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"

#include "frontend/cli.hpp"
#include "frontend/errors.hpp"

namespace mmmcli = mimium::app::cli;

TEST(cli, noarg) {  // NOLINT
  std::vector<const char*> args = {"/usr/local/mimium"};
  auto [appoption, climode] = mmmcli::CliApp::OptionParser()(args.size(), args.data());
  EXPECT_EQ(climode, mmmcli::CliAppMode::Run);
  EXPECT_EQ(appoption.compile_option.stage, mimium::app::CompileStage::Run);
  EXPECT_FALSE(appoption.input.has_value());
  EXPECT_EQ(appoption.output_path, std::nullopt);
  EXPECT_FALSE(appoption.is_verbose);
}

TEST(cli, outputfile) {  // NOLINT
  std::vector<const char*> args = {"/usr/local/mimium", "test_tuple.mmm", "-o", "hoge.mmm"};
  auto [appoption, climode] = mmmcli::CliApp::OptionParser()(args.size(), args.data());
  EXPECT_EQ(climode, mmmcli::CliAppMode::Run);
  EXPECT_EQ(appoption.compile_option.stage, mimium::app::CompileStage::Run);
  EXPECT_EQ(appoption.input.value().filepath, "test_tuple.mmm");
  EXPECT_EQ(appoption.output_path.value(), "hoge.mmm");
  EXPECT_FALSE(appoption.is_verbose);
}

TEST(cli, outputinvalid) {  // NOLINT
  std::vector<const char*> args = {"/usr/local/mimium", "test_tuple.mmm", "-o"};
EXPECT_THROW(mmmcli::CliApp::OptionParser()(args.size(), args.data()), mimium::CliAppError);//NOLINT
}

TEST(cli, optionparsercodegen) {  // NOLINT
  std::vector<const char*> args = {"/usr/local/mimium", "test_tuple.mmm", "--emit-llvm"};
  auto [appoption, climode] = mmmcli::CliApp::OptionParser()(args.size(), args.data());
  EXPECT_EQ(climode, mmmcli::CliAppMode::Run);
  EXPECT_EQ(appoption.compile_option.stage, mimium::app::CompileStage::Codegen);
  EXPECT_EQ(appoption.input.value().filepath, "test_tuple.mmm");
  EXPECT_EQ(appoption.output_path, std::nullopt);
  EXPECT_FALSE(appoption.is_verbose);
}

TEST(cli, optionstage) {  // NOLINT
  std::vector<const char*> args = {"/usr/local/mimium", "test_tuple.mmm", "--emit-mir"};
  auto [appoption, climode] = mmmcli::CliApp::OptionParser()(args.size(), args.data());
  EXPECT_EQ(climode, mmmcli::CliAppMode::Run);
  EXPECT_EQ(appoption.compile_option.stage, mimium::app::CompileStage::MirEmit);
  EXPECT_EQ(appoption.input.value().filepath, "test_tuple.mmm");
  EXPECT_EQ(appoption.output_path, std::nullopt);
  EXPECT_FALSE(appoption.is_verbose);
}
