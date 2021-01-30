
add_executable(RegressionTest regression_test.cpp)
target_compile_features(RegressionTest PRIVATE cxx_std_17)
target_compile_definitions(RegressionTest PRIVATE 
TEST_BIN_DIR=\"${CMAKE_BINARY_DIR}/test\"
)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
target_compile_options(RegressionTest PUBLIC -fsanitize=address -fno-omit-frame-pointer -O0)
target_link_options(RegressionTest PUBLIC -fsanitize=address)
endif()
target_include_directories(RegressionTest
    PRIVATE
    ${MIMIUM_SOURCE_DIR}
    ${GOOGLE_TEST_DIR}/include
    )

target_link_libraries(RegressionTest
  PRIVATE
  mimium_cli
  gtest_main
  )

gtest_discover_tests(
  RegressionTest 
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test)
