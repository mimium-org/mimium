add_library(mimium_utils STATIC ast.cpp mir.cpp type.cpp)
target_compile_options(mimium_utils PUBLIC -std=c++17)
target_link_libraries(mimium_utils PUBLIC)

