cmake_minimum_required(VERSION 3.4)
 
set(CMAKE_CXX_COMPILER /usr/local/opt/llvm/bin/clang++)
set(CMAKE_C_COMPILER /usr/local/opt/llvm/bin/clang) 

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
PROJECT(mimium CXX)

if(${CMAKE_BUILD_TYPE}=="Debug")
    set(LLVM_CONFIG "/usr/local/Cellar/llvm/9.0.0debug/bin/llvm-config")
    list(APPEND CMAKE_PREFIX_PATH "/usr/local/Cellar/llvm/9.0.0debug")
endif()

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})


# for macOS SDK Root
# Including custom cmake rules for clang-tidy and clang-format
include(cmake/clang-cxx-dev-tools.cmake)

#include ccache
include(cmake/EnableCcache.cmake)


# set(RTMIDI_BUILD_TESTING FALSE)
# set(RTMIDI_TARGETNAME_UNINSTALL "uninstall_rtmidi" CACHE STRING "Name of 'uninstall' build target")
# add_subdirectory( libs/rtmidi )

# find_package(SndFile REQUIRED)

#[FIXME] this is unsafe,,,
include_directories(src)

add_custom_target(default_build)

add_subdirectory( src )
add_subdirectory( docs )
#add_subdirectory( test )


install (TARGETS mimium mimium_llloader DESTINATION bin)
install (TARGETS mimium_scheduler mimium_runtime_jit mimium_backend_rtaudio mimium_utils mimium_builtinfn mimium_compiler  DESTINATION lib)
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
