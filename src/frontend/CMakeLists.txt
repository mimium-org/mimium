add_library(mimium_genericapp STATIC genericapp.cpp)
target_link_libraries(mimium_genericapp PUBLIC mimium)
target_compile_features(mimium_genericapp PUBLIC cxx_std_17)
target_include_directories(mimium_genericapp
INTERFACE
$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mimium>
PRIVATE
$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>)

add_library(mimium_cli STATIC cli.cpp)
target_link_libraries(mimium_cli PUBLIC mimium_genericapp)

target_include_directories(mimium_cli
INTERFACE
$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mimium>
PRIVATE
$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>)

