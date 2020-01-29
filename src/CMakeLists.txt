cmake_minimum_required(VERSION 3.8)
add_subdirectory(basic)
add_subdirectory(compiler)
add_subdirectory(runtime)

add_executable(mimium main.cpp)
add_dependencies(default_build mimium)
add_executable(mimium_llloader llloader.cpp)
add_dependencies(default_build mimium_llloader)

target_compile_options(mimium PUBLIC -std=c++17)
target_compile_options(mimium_llloader PUBLIC -std=c++17)

target_include_directories(mimium
PRIVATE
# ${CMAKE_SOURCE_DIR}/libs/rtmidi
# ${SNDFILE_INCLUDE_DIRS}
)
target_include_directories(mimium_llloader
PRIVATE
# ${CMAKE_SOURCE_DIR}/libs/rtmidi
# ${SNDFILE_INCLUDE_DIRS}
)

target_link_libraries(mimium
    mimium_compiler
    mimium_scheduler
    mimium_runtime_jit
    mimium_backend_rtaudio
    )
target_link_libraries(mimium_llloader
    mimium_compiler
    mimium_scheduler
    mimium_runtime_jit
    mimium_backend_rtaudio
    )