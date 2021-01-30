add_library(mimium_preprocessor preprocessor.cpp)
target_include_directories(mimium_preprocessor
INTERFACE
$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mimium>
PRIVATE
$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
)
target_compile_features(mimium_preprocessor PRIVATE cxx_std_17)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
target_compile_options(mimium_preprocessor PUBLIC -fsanitize=address -fno-omit-frame-pointer -O0)
target_link_options(mimium_preprocessor PUBLIC -fsanitize=address)
endif()
target_link_libraries(mimium_preprocessor PRIVATE mimium_filereader)
