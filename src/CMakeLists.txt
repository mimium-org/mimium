cmake_minimum_required(VERSION 3.8)

add_subdirectory(basic)

add_subdirectory(compiler)

add_subdirectory(runtime)

add_executable(mimium main.cpp)

target_compile_options(mimium PUBLIC -std=c++17)
llvm_map_components_to_libnames(mimiumllvm support)
message(STATUS "Components mapped by llvm_config: ${mimiumllvm}")

target_include_directories(mimium
PRIVATE
${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(mimium
    mimium_utils
    mimium_compiler
    mimium_runtime
    LLVM
    )