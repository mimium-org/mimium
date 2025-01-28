include(AddFlexBisonDeps)
set(FLEX_CPP ${FLEX_MyScanner_OUTPUTS}  CACHE PATH "for FLEX outputs ")
set(BISON_CPP ${BISON_MyParser_OUTPUTS}  CACHE PATH "for BISON outputs ")


find_package(SndFile REQUIRED)

#TODO: use ffi in mimium_llloader, mimium_builtinfn must be shared library.
# currently, it fails link dynamically on Windows. 
add_library(mimium_builtinfn ffi.cpp)
target_compile_features(mimium_builtinfn PRIVATE cxx_std_17)
target_include_directories(mimium_builtinfn
INTERFACE
$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mimium>
PRIVATE
$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
$<BUILD_INTERFACE:${SNDFILE_INCLUDE_DIRS}>
)
set_target_properties(mimium_builtinfn PROPERTIES ENABLE_EXPORTS ON)
target_link_libraries(mimium_builtinfn 
PRIVATE
${SNDFILE_LIBRARIES}
mimium_utils )



add_subdirectory(codegen)
add_library(mimium_parser
${FLEX_CPP} 
${BISON_CPP} 
ast_loader.cpp
scanner.cpp
)
target_compile_features(mimium_parser PRIVATE cxx_std_17)

set(PARSER_HEADER_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "for mimium_parser.hpp ")# for location.hh 
set(FLEX_INCLUDE_CACHE ${FLEX_INCLUDE_DIRS} CACHE PATH "to build test")
target_include_directories(mimium_parser
INTERFACE
$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mimium>
PRIVATE
$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
PUBLIC
$<BUILD_INTERFACE:${FLEX_INCLUDE_CACHE}>
$<BUILD_INTERFACE:${PARSER_HEADER_DIR}>
)

add_library(mimium_compiler

mirgenerator.cpp 
ast_lowering.cpp
type_lowering.cpp
ast_loader.cpp
type_infer_visitor.cpp 
closure_convert.cpp 
collect_memoryobjs.cpp 
compiler.cpp

)

target_include_directories(mimium_compiler
PRIVATE
$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mimium>
$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
$<BUILD_INTERFACE:${LLVM_INCLUDE_DIRS}>
)
target_compile_features(mimium_compiler PRIVATE cxx_std_17)


target_link_libraries(mimium_compiler 
PRIVATE
mimium_parser
mimium_utils
mimium_filereader
mimium_builtinfn
mimium_llvm_codegen
)