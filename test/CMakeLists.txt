cmake_minimum_required(VERSION 3.0)
add_subdirectory(googletest)

file(GLOB SRCS ./*.cpp)
add_executable(UnitTest ${SRCS})
target_compile_options(UnitTest PUBLIC -std=c++17)
target_compile_definitions(UnitTest PRIVATE
TEST_ROOT_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\"
)
target_include_directories(UnitTest
    PRIVATE
    .
    ${GTEST_DIR}/include
    ${FLEX_INCLUDE_CACHE}
    ${PARSER_HEADER_DIR}
    )

target_link_libraries(UnitTest
  PRIVATE
    gtest_main
    mimium_utils
    mimium_preprocessor
    mimium_compiler
    LLVMCore
  )

add_test(Name UnitTest Command "ASAN_OPTIONS=detect_container_overflow=0 ${CMAKE_CURRENT_BINARY_DIR}/UnitTest")

file(GLOB_RECURSE testsource LIST_DIRECTORIES true mmm/*.mmm )
file(GLOB_RECURSE testassets LIST_DIRECTORIES true ${CMAKE_SOURCE_DIR}/assets/*.wav)


file(COPY ${testsource} ${testassets} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(regression)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
add_subdirectory(fuzzing)
endif()