cmake_minimum_required(VERSION 3.0)


set(INSTALL_GTEST OFF)
add_subdirectory(googletest)
include(GoogleTest)

BISON_TARGET(TestParser ${CMAKE_SOURCE_DIR}/src/compiler/mimium.yy ${CMAKE_BINARY_DIR}/src/compiler/mimium_parser_test.cpp
DEFINES_FILE ${CMAKE_BINARY_DIR}/test/mimium_parser.hpp
)
FLEX_TARGET(TestScanner ${CMAKE_SOURCE_DIR}/src/compiler/mimium.l ${CMAKE_BINARY_DIR}/src/compiler/tokens_test.cpp)

ADD_FLEX_BISON_DEPENDENCY(TestScanner TestParser)




set (MIMIUM_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set (GOOGLETEST_DIR ${CMAKE_SOURCE_DIR}/test/googletest/googletest)
add_library(TestLib STATIC 
${MIMIUM_SOURCE_DIR}/basic/mir.cpp
${MIMIUM_SOURCE_DIR}/basic/filereader.cpp
${FLEX_TestScanner_OUTPUTS}
${BISON_TestParser_OUTPUTS}
${MIMIUM_SOURCE_DIR}/compiler/ast_loader.cpp
${MIMIUM_SOURCE_DIR}/compiler/scanner.cpp
${MIMIUM_SOURCE_DIR}/compiler/ast_lowering.cpp
${MIMIUM_SOURCE_DIR}/compiler/type_lowering.cpp
${MIMIUM_SOURCE_DIR}/compiler/type_infer_visitor.cpp
${MIMIUM_SOURCE_DIR}/compiler/mirgenerator.cpp
# ${MIMIUM_SOURCE_DIR}/frontend/genericapp.cpp
# ${MIMIUM_SOURCE_DIR}/frontend/cli.cpp
)
target_include_directories(TestLib
    PUBLIC
    .
    $<BUILD_INTERFACE:${FLEX_INCLUDE_CACHE}>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src/compiler>
    $<BUILD_INTERFACE:${MIMIUM_SOURCE_DIR}>
    )
target_compile_features(TestLib PRIVATE cxx_std_17)

function(MakeTest TestName mainsrc)

add_executable(${TestName} ${mainsrc})
  foreach(arg IN LISTS ARGN)
    target_sources(${TestName} PRIVATE ${arg})
  endforeach()
  target_compile_definitions(${TestName} PRIVATE
    TEST_ROOT_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\")
    target_compile_features(${TestName} PRIVATE cxx_std_17)
    target_include_directories(${TestName}
    PRIVATE
    .
    ${GOOGLETEST_DIR}/include
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    )
  target_link_libraries(${TestName} PRIVATE gtest_main mimium_builtinfn TestLib)
  gtest_discover_tests(${TestName} WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test)
endfunction(MakeTest)

MakeTest(AstTest 0.newast_test.cpp)
MakeTest(AstToStringTest 1.ast_to_string_test.cpp)
MakeTest(ParserTest 2.parser_test.cpp)
MakeTest(SymbolRenameTest 3.symbolrename_test.cpp)
MakeTest(TypeInferTest 4.typeinfer_test.cpp)
MakeTest(MirgenTest 5.mirgen_test.cpp)
MakeTest(AstlowererTest ast_lowering_test.cpp)

add_executable(CliAppTest 6.cli_test.cpp)
target_compile_features(CliAppTest PRIVATE cxx_std_17)
target_compile_definitions(CliAppTest PRIVATE TEST_ROOT_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\")
target_include_directories(CliAppTest PRIVATE ${GOOGLETEST_DIR}/include $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>)
target_link_libraries(CliAppTest PRIVATE gtest_main mimium_cli)
gtest_discover_tests(CliAppTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test)

if(ENABLE_COVERAGE)
  add_custom_target(Lcov
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMAND lcov -c -d . -o lcov.info
  COMMAND lcov -r lcov.info "*/googletest/*" "*/c++/*" "/usr/local/*" -o lcov.info
  BYPRODUCTS ${CMAKE_BINARY_DIR}/lcov.info
  )
  add_custom_target(LcovResetCounter
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMAND lcov --zerocounters -d .
  )
endif()

file(GLOB_RECURSE testsource LIST_DIRECTORIES true mmm/*.mmm )
file(GLOB_RECURSE testassets LIST_DIRECTORIES true ${CMAKE_SOURCE_DIR}/assets/*.wav)


file(COPY ${testsource} ${testassets} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(regression)

add_custom_target(Tests)
add_dependencies(Tests 
AstTest
AstToStringTest
ParserTest
SymbolRenameTest
TypeInferTest
MirgenTest
CliAppTest
RegressionTest)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
add_subdirectory(fuzzing)
endif()