
add_library(mimium_runtime STATIC audiodriver.cpp scheduler.cpp runtime.cpp)

target_compile_options(mimium_runtime PUBLIC -std=c++17)

target_include_directories(mimium_runtime
PRIVATE
${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(mimium_runtime mimium_utils rtaudio ${SNDFILE_LIBRARIES} LLVMExecutionEngine)
