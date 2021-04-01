#include <cstdlib>
#include "utils/include_filesystem.hpp"

#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"

// regression test. test files should be put on test/mmm folder.
// test files must put some answer to stdout through print() function in mimium code.
// file name of test must be "test_xxxxxx.mmm" and then test name should be xxxxxx.

#ifndef TEST_BIN_DIR
#define TEST_BIN_DIR ""
#endif
// NOLINTNEXTLINE
#define REGRESSION(filename, expect)                                                          \
  TEST(regression, filename) { /*NOLINT*/                                                     \
    testing::internal::CaptureStdout();                                                       \
    fs::path testbinpath(TEST_BIN_DIR);                                                       \
    fs::current_path(testbinpath);                                                            \
    fs::path bin = testbinpath.parent_path() / fs::path("src/mimium");                        \
    fs::path filepath = testbinpath / fs::path("test_" #filename ".mmm");                     \
    std::string command =                                                                     \
        "ASAN_OPTIONS=detect_container_overflow=0 " + bin.string() + " " + filepath.string(); \
    std::system(command.c_str());                                                             \
    std::string output = testing::internal::GetCapturedStdout();                              \
    EXPECT_STREQ(output.c_str(), expect);                                                     \
  }

REGRESSION(regression, "120")
REGRESSION(operators,"161011011100832-20\n")
REGRESSION(typeident, R"(3
3
2
)")
REGRESSION(closure2, "20015\n")
REGRESSION(tuple, "100\n")
REGRESSION(fibonacchi, "610\n")
REGRESSION(ifexpr, "130\n")
REGRESSION(if_void,"1\n2\n2\n")
REGRESSION(builtin,
           R"(100
2.55255e+08
0.932039
0.362358
2.57215
0.100167
1.36944
0.876058
0.44752
1.50946
1.81066
0.833655
0.182322
0.0791812
3.04092
1.41421
0.55
2
1
1
1
2
20.22
0.555
0.234563
100
200
)")

REGRESSION(libsndfile, "146640\n-0.0803833\n")
REGRESSION(tuple_capture, "100\n200\n300\n")
REGRESSION(tupletofn, "0.2\n0.8\n0.6\n2.4\n")
REGRESSION(array, "100\n200\n300\n400\n500\n")
REGRESSION(array_capture, "100\n200\n300\n400\n500\n")
REGRESSION(array_tofun, "100\n200\n300\n400\n500\n")
REGRESSION(arrayreturn, "100\n200\n300\n400\n500\n")
REGRESSION(arraylvar, "600\n700\n800\n")

REGRESSION(structtype, "999\n")
REGRESSION(typealias, "100\n200\n100\n")