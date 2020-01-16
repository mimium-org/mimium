cmake_minimum_required(VERSION 3.8)
PROJECT(mimium CXX)

add_subdirectory(basic)

add_subdirectory(compiler)

add_subdirectory(runtime)

add_executable(mimium main.cpp)

target_compile_options(mimium PUBLIC -std=c++17)

target_include_directories(mimium
PRIVATE
${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(mimium
    mimium_utils
    mimium_compiler
    mimium_runtime
    LLVMSupport #for commandline library
    )