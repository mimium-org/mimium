add_library(mimium_preprocessor preprocessor.cpp)
target_include_directories(mimium_preprocessor
INTERFACE
$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mimium>
PRIVATE
$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
)
target_compile_features(mimium_preprocessor PRIVATE cxx_std_17)
target_link_libraries(mimium_preprocessor PRIVATE mimium_filereader)
