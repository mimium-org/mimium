
add_library(mimium_utils helper_functions.cpp ast.cpp mir.cpp type.cpp)

target_compile_options(mimium_utils PUBLIC -std=c++17)

llvm_map_components_to_libnames(llvmsp support)
message(STATUS "Components mapped by llvm_config: ${llvmsp}")
target_include_directories(mimium_utils
    PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(mimium_utils LLVM)
