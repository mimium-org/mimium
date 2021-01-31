add_library(mimium_genericapp genericapp.cpp)
target_link_libraries(mimium_genericapp PRIVATE mimium)
target_compile_features(mimium_genericapp PUBLIC cxx_std_17)
target_include_directories(mimium_genericapp
INTERFACE
$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mimium>
PRIVATE
${LLVM_INCLUDE_DIRS}
$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>)

add_library(mimium_cli cli.cpp)
target_link_libraries(mimium_cli PRIVATE mimium_genericapp mimium)
target_compile_features(mimium_cli PUBLIC cxx_std_17)
target_include_directories(mimium_cli
INTERFACE
$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mimium>
PRIVATE
$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>)
