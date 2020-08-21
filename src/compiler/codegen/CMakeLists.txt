llvm_map_components_to_libnames(compilerllvm core)
message(STATUS "Components mapped by llvm_config: ${compilerllvm}")

add_library(mimium_llvm_codegen STATIC llvmgenerator.cpp typeconverter.cpp codegen_visitor.cpp)
target_compile_options(mimium_llvm_codegen PUBLIC -std=c++17)

target_include_directories(mimium_llvm_codegen 
PUBLIC
${MIMIUM_SOURCE_DIR}
)
target_link_libraries(mimium_llvm_codegen PRIVATE mimium_utils mimium_builtinfn ${compilerllvm})