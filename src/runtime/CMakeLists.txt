add_library(mimium_runtime STATIC scheduler.cpp runtime.cpp)

add_subdirectory(backend)


target_compile_options(mimium_runtime PUBLIC -std=c++17)


llvm_map_components_to_libnames(llvmruntime orcjit native )
message(STATUS "Components mapped by llvm_config: ${llvmruntime}")
target_include_directories(mimium_runtime
PRIVATE
#  ${CMAKE_SOURCE_DIR}/libs/rtaudio
#  ${CMAKE_SOURCE_DIR}/libs/rtmidi 
${SNDFILE_INCLUDE_DIRS}
${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(mimium_runtime mimium_utils mimium_backend_rtaudio ${SNDFILE_LIBRARIES} LLVM)
