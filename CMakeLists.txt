cmake_minimum_required(VERSION 3.4)
option(BUILD_DOCS "build a documentation")
option(BUILD_TEST "build a test" OFF)

#set(CMAKE_CXX_COMPILER /usr/local/opt/llvm/bin/clang++)
#set(CMAKE_C_COMPILER /usr/local/opt/llvm/bin/clang) 


set_property(GLOBAL PROPERTY USE_FOLDERS ON)
PROJECT(mimium 
LANGUAGES CXX
HOMEPAGE_URL "https://mimium.org"
VERSION 0.1.2
)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    set(CMAKE_CXX_FLAGS_DEBUG "-g4")
	# set(CMAKE_STATIC_LIBRARY_SUFFIX ".js")
    # set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()
#if(${CMAKE_BUILD_TYPE}=="Debug")
#    set(LLVM_CONFIG "/usr/local/Cellar/llvm/9.0.0debug/bin/llvm-config")
#endif()

message(STATUS "set llvm dir to ${LLVM_DIR}")
set(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
SET(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)
# list(APPEND CMAKE_PREFIX_PATH $ENV{LLVM_DIR})

find_package(LLVM  REQUIRED CONFIG PATHS ${LLVM_DIR})
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

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

elseif(CMAKE_EXPORT_COMPILE_COMMANDS)
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

add_custom_target(default_build)

add_subdirectory( src )
if(BUILD_DOCS)
add_subdirectory( docs )
endif()

if(NOT(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten") AND ${BUILD_TEST})
enable_testing()
add_subdirectory( test )
endif()

