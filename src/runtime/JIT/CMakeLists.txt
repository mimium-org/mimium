add_library(mimium_runtime_jit STATIC runtime_jit.cpp)

target_compile_options(mimium_runtime_jit PUBLIC -std=c++17)

llvm_map_components_to_libnames(llvmruntime orcjit native )
message(STATUS "Components mapped by llvm_config: ${llvmruntime}")
target_include_directories(mimium_runtime_jit
PRIVATE
#  ${CMAKE_SOURCE_DIR}/libs/rtaudio
#  ${CMAKE_SOURCE_DIR}/libs/rtmidi 
# ${SNDFILE_INCLUDE_DIRS}
# ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(mimium_runtime_jit mimium_scheduler ${llvmruntime})

install(TARGETS mimium_runtime_jit DESTINATION lib)