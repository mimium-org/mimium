if(NOT(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten"))
add_subdirectory(rtaudio)
endif()


