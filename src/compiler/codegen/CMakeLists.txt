add_library(mimium_llvm_codegen STATIC
    llvmgenerator.cpp 
    typeconverter.cpp 
    codegen_visitor.cpp)
target_compile_features(mimium_llvm_codegen PUBLIC cxx_std_17)

target_include_directories(mimium_llvm_codegen 
PUBLIC
${MIMIUM_SOURCE_DIR}
${LLVM_INCLUDE_DIRS}
)
target_link_libraries(mimium_llvm_codegen 
PRIVATE 
mimium_utils mimium_builtinfn 
PUBLIC
${LLVM_LIBRARIES})