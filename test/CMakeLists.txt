cmake_minimum_required(VERSION 3.0)
project(Test CXX)

find_package(GTest REQUIRED)
include(GoogleTest)
add_executable(UnitTest newast_test.cpp)
target_compile_options(UnitTest PUBLIC -std=c++17)
llvm_map_components_to_libnames(test_llvm core)
message(STATUS "Components mapped by llvm_config: ${test_llvm}")

target_include_directories(UnitTest
    PRIVATE
    .
    ${GTEST_INCLUDE_DIRS}
    )

target_link_libraries(UnitTest
  PRIVATE
    GTest::GTest
    GTest::Main   # GTest::Mainをリンクすると自分でmain関数を書く必要がなくなる
    mimium_utils
    ${test_llvm}
  )

add_test(Name UnitTest Command UnitTest)

# file(GLOB testsource *.mmm)
# file(COPY ${testsource} DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )