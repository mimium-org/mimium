cmake_minimum_required(VERSION 3.4)

add_library(mimium_utils STATIC helper_functions.cpp ast.cpp mir.cpp)

target_include_directories(mimium_utils
    PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(mimium_utils ${llvm_libs})
