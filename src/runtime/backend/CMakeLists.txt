add_library(mimium_audiodriver STATIC audiodriver.cpp)

target_include_directories(mimium_audiodriver
PUBLIC
${MIMIUM_SOURCE_DIR}
)
target_compile_options(mimium_audiodriver PRIVATE -std=c++17)

target_link_libraries(mimium_audiodriver PUBLIC mimium_scheduler)

if(NOT(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten"))
add_subdirectory(rtaudio)
endif()


