add_library(mimium_runtime_jit STATIC runtime_jit.cpp)

target_compile_options(mimium_runtime_jit PUBLIC -std=c++17)
add_dependencies(mimium_runtime_jit mimium_utils)
target_include_directories(mimium_runtime_jit
PUBLIC
${LLVM_INCLUDE_DIRS}
PRIVATE
${MIMIUM_SOURCE_DIR}
)
target_compile_options(mimium_runtime_jit PUBLIC
${LLVM_CXX_FLAGS})

target_link_libraries(mimium_runtime_jit 
PUBLIC 
PRIVATE
${LLVM_LIBRARIES}
mimium_scheduler 
)
target_link_options(mimium_runtime_jit PUBLIC
${LLVM_LD_FLAGS})

install(TARGETS mimium_runtime_jit DESTINATION lib)