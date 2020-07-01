cmake_minimum_required(VERSION 3.4)
option(BUILD_DOCS "build a documentation")


#set(CMAKE_CXX_COMPILER /usr/local/opt/llvm/bin/clang++)
#set(CMAKE_C_COMPILER /usr/local/opt/llvm/bin/clang) 

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
PROJECT(mimium CXX)

#if(${CMAKE_BUILD_TYPE}=="Debug")
#    set(LLVM_CONFIG "/usr/local/Cellar/llvm/9.0.0debug/bin/llvm-config")
#    list(APPEND CMAKE_PREFIX_PATH "/usr/local/Cellar/llvm/9.0.0debug")
#endif()

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

add_compile_options("$<$<CONFIG:DEBUG>:-DMIMIUM_DEBUG_BUILD>")

# set(RTMIDI_BUILD_TESTING FALSE)
# set(RTMIDI_TARGETNAME_UNINSTALL "uninstall_rtmidi" CACHE STRING "Name of 'uninstall' build target")
# add_subdirectory( libs/rtmidi )
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")


#[FIXME] this is unsafe,,,
include_directories(src)

add_custom_target(default_build)

add_subdirectory( src )
if(BUILD_DOCS)
add_subdirectory( docs )
endif()

#add_subdirectory( test )


