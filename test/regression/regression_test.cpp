#include <cstdlib>
#include <filesystem>
namespace fs = std::filesystem;

#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"

// regression test. test files should be put on test/mmm folder.
// test files must put some answer to stdout through print() function in mimium code.
// file name of test must be "test_xxxxxx.mmm" and then test name should be xxxxxx.

#ifndef TEST_BIN_DIR
#define TEST_BIN_DIR ""
#endif

#define REGRESSION(filename, expect)                                                         \
  TEST(regression, filename) {                                                               \
    testing::internal::CaptureStdout();                                                      \
    fs::path cwd = fs::current_path();                                                       \
    fs::path testbinpath(TEST_BIN_DIR);                                                      \
    fs::path bin = testbinpath.parent_path().parent_path() / fs::path("src/mimium");         \
    fs::path filepath = testbinpath.parent_path() / fs::path("test_" #filename ".mmm");      \
    std::string command =                                                                    \
        "ASAN_OPTIONS=detect_container_overflow=0 " + bin.string() + " " + filepath.string(); \
    std::system(command.c_str());                                                            \
    std::string output = testing::internal::GetCapturedStdout();                             \
    EXPECT_STREQ(output.c_str(), expect);                                                    \
  }

REGRESSION(regression, "120")

REGRESSION(closure2, "20015")