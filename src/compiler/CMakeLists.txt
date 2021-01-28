include(AddFlexBisonDeps)

find_package(SndFile REQUIRED)

#TODO: use ffi in mimium_llloader, mimium_builtinfn must be shared library.
# currently, it fails link dynamically on Windows. 
add_library(mimium_builtinfn SHARED ffi.cpp)
target_compile_features(mimium_builtinfn PRIVATE cxx_std_17)
target_include_directories(mimium_builtinfn
PRIVATE
${SNDFILE_INCLUDE_DIRS}
)

target_link_libraries(mimium_builtinfn 
PUBLIC 
${SNDFILE_LIBRARIES}
PRIVATE
mimium_utils )


add_subdirectory(codegen)
add_library(mimium_parser STATIC
${FLEX_MyScanner_OUTPUTS} 
${BISON_MyParser_OUTPUTS} 
ast_loader.cpp)
target_compile_features(mimium_parser PRIVATE cxx_std_17)

set(PARSER_HEADER_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "for mimium_parser.hpp ")# for location.hh 
set(FLEX_INCLUDE_CACHE ${FLEX_INCLUDE_DIRS} CACHE PATH "to build test")
target_include_directories(mimium_parser
PUBLIC
${FLEX_INCLUDE_CACHE}
${PARSER_HEADER_DIR} 
PRIVATE
${MIMIUM_SOURCE_DIR}
)

add_library(mimium_compiler SHARED
symbolrenamer.cpp 
mirgenerator.cpp 
type_infer_visitor.cpp 
closure_convert.cpp 
collect_memoryobjs.cpp 
compiler.cpp)

target_include_directories(mimium_compiler
PUBLIC
${MIMIUM_SOURCE_DIR}
)
target_compile_features(mimium_compiler PRIVATE cxx_std_17)


target_link_libraries(mimium_compiler PUBLIC
mimium_parser
mimium_llvm_codegen 
mimium_utils
mimium_filereader
mimium_builtinfn)

install(TARGETS mimium_compiler mimium_builtinfn DESTINATION lib)