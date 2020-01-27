
add_library(mimium_llvm_codegen STATIC llvmgenerator.cpp typeconverter.cpp)
target_compile_options(mimium_llvm_codegen PUBLIC -std=c++17)

target_include_directories(mimium_llvm_codegen 
PRIVATE
)

target_link_libraries(mimium_llvm_codegen mimium_utils LLVM)