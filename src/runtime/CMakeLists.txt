
add_library(mimium_runtime audiodriver.cpp scheduler.cpp runtime.cpp)

target_compile_options(mimium_runtime PUBLIC -std=c++17)

llvm_map_components_to_libnames(llvmruntime orcjit native )
message(STATUS "Components mapped by llvm_config: ${llvmruntime}")
target_include_directories(mimium_runtime
PRIVATE
${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(mimium_runtime mimium_utils rtaudio ${SNDFILE_LIBRARIES} ${llvmruntime})
