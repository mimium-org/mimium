list(APPEND CMAKE_PREFIX_PATH  /usr/local /usr/local/opt/flex)
list(APPEND CMAKE_PREFIX_PATH  /usr/local /usr/local/opt/bison)
find_package(BISON 3.3 REQUIRED)
find_package(FLEX 2.6 REQUIRED)

BISON_TARGET(MyParser mimium.yy ${CMAKE_CURRENT_BINARY_DIR}/mimium_parser.cpp
DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/mimium_parser.hpp
VERBOSE REPORT_FILE ${CMAKE_CURRENT_BINARY_DIR}/bison.log
)
FLEX_TARGET(MyScanner mimium.l ${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp)

ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)

find_package(SndFile REQUIRED)

add_library(mimium_builtinfn SHARED ffi.cpp)
target_include_directories(mimium_builtinfn
PUBLIC
${SNDFILE_INCLUDE_DIRS}
)
target_link_libraries(mimium_builtinfn PUBLIC mimium_utils  ${SNDFILE_LIBRARIES})


add_subdirectory(codegen)
add_library(mimium_parser ${FLEX_MyScanner_OUTPUTS} ${BISON_MyParser_OUTPUTS} ast_loader_new.cpp  )
target_compile_options(mimium_parser PUBLIC -std=c++17)


set(PARSER_HEADER_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "for mimium_parser.hpp ")# for location.hh 
set(FLEX_INCLUDE_CACHE ${FLEX_INCLUDE_DIRS})
target_include_directories(mimium_parser
PUBLIC
${FLEX_INCLUDE_DIRS}
${PARSER_HEADER_DIR} 
)

add_library(mimium_compiler  recursive_checker.cpp alphaconvert_visitor.cpp knormalize_visitor.cpp type_infer_visitor.cpp closure_convert.cpp collect_memoryobjs.cpp compiler.cpp)
add_dependencies(mimium_compiler  mimium_parser mimium_builtinfn)
target_include_directories(mimium_compiler
PUBLIC
${FLEX_INCLUDE_DIRS}
${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(mimium_compiler PRIVATE mimium_llvm_codegen mimium_utils mimium_builtinfn)

install(TARGETS mimium_compiler mimium_builtinfn DESTINATION lib)