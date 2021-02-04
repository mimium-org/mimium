add_library(mimium_llvm_codegen STATIC
    llvmgenerator.cpp 
    typeconverter.cpp 
    codegen_visitor.cpp)
target_compile_features(mimium_llvm_codegen PUBLIC cxx_std_17)

target_include_directories(mimium_llvm_codegen 
INTERFACE
$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mimium>
PRIVATE
$<BUILD_INTERFACE:${LLVM_INCLUDE_DIRS}>
$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>)
target_link_libraries(mimium_llvm_codegen 
PRIVATE 
mimium_utils mimium_builtinfn 
$<BUILD_INTERFACE:${LLVM_LIBRARIES}>)