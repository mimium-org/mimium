list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/flex)
list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/bison)
find_package(BISON 3.2 REQUIRED)
find_package(Flex REQUIRED)

BISON_TARGET(MyParser mimium.yy ${CMAKE_CURRENT_BINARY_DIR}/mimium_parser.cpp
DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/mimium_parser.hpp
VERBOSE REPORT_FILE ${CMAKE_CURRENT_BINARY_DIR}/bison.log
)
FLEX_TARGET(MyScanner mimium.l ${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp)

ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)

add_library(mimium_compiler ${FLEX_MyScanner_OUTPUTS} ${BISON_MyParser_OUTPUTS} driver.cpp recursive_checker.cpp alphaconvert_visitor.cpp knormalize_visitor.cpp type_infer_visitor.cpp closure_convert.cpp collect_memoryobjs.cpp  ffi.cpp compiler.cpp)

add_subdirectory(codegen)

llvm_map_components_to_libnames(compilerllvm core orcjit)
message(STATUS "Components mapped by llvm_config: ${compilerllvm}")
target_compile_options(mimium_compiler PUBLIC -std=c++17)

target_include_directories(mimium_compiler
PRIVATE
${FLEX_INCLUDE_DIRS}
${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(mimium_compiler mimium_llvm_codegen mimium_utils LLVM)