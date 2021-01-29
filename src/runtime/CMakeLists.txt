add_library(mimium_scheduler SHARED scheduler.cpp)
target_compile_features(mimium_scheduler PUBLIC cxx_std_17)
target_include_directories(mimium_scheduler 
INTERFACE
$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mimium>
PRIVATE
$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
)

target_link_libraries(mimium_scheduler PRIVATE 
mimium_utils)

add_subdirectory(backend)
add_subdirectory(JIT)