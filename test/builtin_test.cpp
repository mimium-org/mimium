#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"

#include "builtin_functions.hpp"

TEST(builtin_test, print) {
    testing::internal::CaptureStdout();
    mimium::builtin::print("test");
    ASSERT_STREQ("test", testing::internal::GetCapturedStdout().c_str());

}
