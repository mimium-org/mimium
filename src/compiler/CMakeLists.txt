BISON_TARGET(MyParser mimium.yy ${CMAKE_CURRENT_BINARY_DIR}/mimium_parser.cpp)
FLEX_TARGET(MyScanner mimium.l ${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp)
ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)

add_library(mimium_compiler STATIC ${FLEX_MyScanner_OUTPUTS} ${BISON_MyParser_OUTPUTS} driver.cpp recursive_checker.cpp alphaconvert_visitor.cpp knormalize_visitor.cpp type_infer_visitor.cpp closure_convert.cpp llvmgenerator.cpp ffi.cpp compiler.cpp)

target_compile_options(mimium_compiler PUBLIC -std=c++17)

target_include_directories(mimium_compiler
PRIVATE
${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(mimium_compiler mimium_utils  ${llvm_libs})