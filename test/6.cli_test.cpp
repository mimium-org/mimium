
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"

#include "frontend/cli.hpp"
namespace mmmcli = mimium::app::cli;
TEST(cli, optionparser) {  // NOLINT
  std::vector<const char*> args = {"/usr/local/mimium", "test_tuple.mmm", "--emit-llvm"};
  auto [appoption, climode] = mmmcli::CliApp::OptionParser()(args.size(), args.data());
  EXPECT_EQ(climode, mmmcli::CliAppMode::Run);
  EXPECT_EQ(appoption.compile_option.stage, mimium::app::CompileStage::Codegen);
  EXPECT_EQ(appoption.input.value().filepath, "test_tuple.mmm");
  EXPECT_EQ(appoption.output_path, std::nullopt);
  EXPECT_FALSE(appoption.is_verbose);
}
