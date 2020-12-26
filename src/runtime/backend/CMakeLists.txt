add_library(mimium_audiodriver STATIC audiodriver.cpp)

target_include_directories(mimium_audiodriver
PUBLIC
${MIMIUM_SOURCE_DIR}
)
target_compile_features(mimium_audiodriver PUBLIC cxx_std_17)
target_link_libraries(mimium_audiodriver PRIVATE
mimium_scheduler)

if(NOT(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten"))
add_subdirectory(rtaudio)
endif()


