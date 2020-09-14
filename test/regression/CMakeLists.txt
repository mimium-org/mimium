
add_executable(RegressionTest regression_test.cpp)
target_compile_features(RegressionTest PRIVATE cxx_std_17)
target_compile_definitions(RegressionTest PRIVATE 
TEST_BIN_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\"
)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
target_compile_options(RegressionTest PUBLIC -fsanitize=address -fno-omit-frame-pointer -O0)
target_link_options(RegressionTest PUBLIC -fsanitize=address)
endif()
target_include_directories(RegressionTest
    PRIVATE
    ${GTEST_INCLUDE_DIRS}
    )

target_link_libraries(RegressionTest
  PRIVATE
    GTest::GTest
    GTest::Main   # GTest::Mainをリンクすると自分でmain関数を書く必要がなくなる
  )

add_test(Name RegressionTest Command RegressionTest)
