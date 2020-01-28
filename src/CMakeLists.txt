cmake_minimum_required(VERSION 3.8)
add_subdirectory(basic)
add_subdirectory(compiler)
add_subdirectory(runtime)

add_executable(mimium main.cpp)

add_executable(mimium_llloader llloader.cpp)

target_compile_options(mimium PUBLIC -std=c++17)
target_compile_options(mimium_llloader PUBLIC -std=c++17)


llvm_map_components_to_libnames(mimiumllvm support)
message(STATUS "Components mapped by llvm_config: ${mimiumllvm}")

target_include_directories(mimium
PRIVATE
${CMAKE_SOURCE_DIR}/externals/rtaudio
# ${CMAKE_SOURCE_DIR}/libs/rtmidi
# ${SNDFILE_INCLUDE_DIRS}
#to include bison/flex-generated files
${FLEX_INCLUDE_DIR}
${CMAKE_CURRENT_BINARY_DIR}/compiler
)
target_include_directories(mimium_llloader
PRIVATE
${CMAKE_SOURCE_DIR}/externals/rtaudio
# ${CMAKE_SOURCE_DIR}/libs/rtmidi
# ${SNDFILE_INCLUDE_DIRS}
#to include bison/flex-generated files
${FLEX_INCLUDE_DIR}
${CMAKE_CURRENT_BINARY_DIR}/compiler
)

target_link_libraries(mimium
    mimium_utils
    mimium_compiler
    mimium_scheduler
    mimium_runtime_jit
    mimium_backend_rtaudio
    LLVM
    )
    target_link_libraries(mimium_llloader
    mimium_utils
    mimium_compiler
    mimium_scheduler
    mimium_runtime_jit
    mimium_backend_rtaudio
    LLVM
    )