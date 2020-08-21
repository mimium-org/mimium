cmake_minimum_required(VERSION 3.8)
project(Main CXX)

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
${FLEX_INCLUDE_CACHE}
${PARSER_HEADER_DIR} 

)
target_include_directories(mimium_llloader
PRIVATE
# ${CMAKE_SOURCE_DIR}/libs/rtmidi
# ${SNDFILE_INCLUDE_DIRS}
${FLEX_INCLUDE_CACHE}
${PARSER_HEADER_DIR} 
)
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
target_link_options(mimium  PUBLIC -stdlib=libc++ -lc++abi)
target_link_options(mimium_llloader  PUBLIC -stdlib=libc++ -lc++abi)
endif()

target_link_libraries(mimium
    LLVMCore
    mimium_compiler
    mimium_scheduler
    mimium_runtime_jit
    mimium_backend_rtaudio
    mimium_builtinfn 
    )
target_link_libraries(mimium_llloader
    mimium_compiler
    mimium_scheduler
    mimium_runtime_jit
    mimium_backend_rtaudio
    mimium_builtinfn 
    )


install (TARGETS mimium mimium_llloader DESTINATION bin)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/" # source directory
             DESTINATION "include" # target directory
             FILES_MATCHING # install only matched files
             PATTERN "*.hpp" # select header files
     )
     
    add_custom_target(uninstall)
    add_custom_command(TARGET uninstall
    COMMAND cat install_manifest.txt | sudo xargs rm -f
    COMMAND cat install_manifest.txt | xargs -L1 dirname | sudo xargs  rmdir -p 2&> /dev/null
    )