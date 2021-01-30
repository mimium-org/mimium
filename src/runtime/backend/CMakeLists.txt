add_library(mimium_audiodriver audiodriver.cpp)

target_include_directories(mimium_audiodriver
PRIVATE
$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mimium>
$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
)
target_compile_features(mimium_audiodriver PUBLIC cxx_std_17)
target_link_libraries(mimium_audiodriver PRIVATE
mimium_scheduler)

if(NOT(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten"))
add_subdirectory(rtaudio)
endif()


