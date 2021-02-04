cmake_minimum_required(VERSION 3.4)
option(BUILD_DOCS "build a documentation")
option(BUILD_TEST "build a test" OFF)
option(ENABLE_LLD "use lld for linker" OFF)
option(ENABLE_COVERAGE "Generate code coverage data for gcov" OFF)
option(BUILD_SHARED_LIBS "build libraries as a dynamic link libraries" OFF)


#set(CMAKE_CXX_COMPILER /usr/local/opt/llvm/bin/clang++)
#set(CMAKE_C_COMPILER /usr/local/opt/llvm/bin/clang) 


set_property(GLOBAL PROPERTY USE_FOLDERS ON)
PROJECT(mimium 
LANGUAGES CXX
HOMEPAGE_URL "https://mimium.org"
VERSION 0.3.0
)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    set(CMAKE_CXX_FLAGS_DEBUG "-g4")
	# set(CMAKE_STATIC_LIBRARY_SUFFIX ".js")
    # set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

if(NOT(APPLE)  AND  ${ENABLE_LLD} AND (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang"))#sadly lld will not work with macos
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld)
set(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=lld)
set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld)
endif()

if(ENABLE_COVERAGE)
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} --coverage)
  set(CMAKE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} --coverage)
endif()

if((CMAKE_BUILD_TYPE STREQUAL "Debug") AND 
    (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
message("sanitizer activated")
list(APPEND CMAKE_CXX_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_LINKER_FLAGS ${CMAKE_LINKER_FLAGS} -fsanitize=address)
endif()

set(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
SET(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)
# list(APPEND CMAKE_PREFIX_PATH $ENV{LLVM_DIR})

include(cmake/FindLlvm.cmake)
message(STATUS "set llvm include dir to ${LLVM_INCLUDE_DIRS}, libdir to ${LLVM_LIBDIRS}")

execute_process(COMMAND ${LLVM_CONFIG_EXE} --cppflags
OUTPUT_VARIABLE LLVM_CXX_FLAGS
OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG_EXE} --ldflags
OUTPUT_VARIABLE LLVM_LD_FLAGS
OUTPUT_STRIP_TRAILING_WHITESPACE)


SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})


# for macOS SDK Root
# Including custom cmake rules for clang-tidy and clang-format
# disable on windows because glob_recurse takes too long time
if(APPLE)
include(cmake/clang-cxx-dev-tools.cmake)
endif()
#include ccache
include(cmake/EnableCcache.cmake)

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

elseif(CMAKE_EXPORT_COMPILE_COMMANDS)
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()




if(MINGW OR MSYS)
if(NOT CMAKE_COMPILER_IS_GNUCXX)
message(WARNING "Currently, only g++ is allowed to build on mingw/msys to link standard library statically.")
endif()
set(CMAKE_CXX_STANDARD_LIBRARIES "-static -lstdc++ -lgcc -lz -lwinpthread ${CMAKE_CXX_STANDARD_LIBRARIES}")
endif()

if(UNIX AND NOT(APPLE OR MINGW OR MSYS))
set(CMAKE_CXX_STANDARD_LIBRARIES "-lz -ldl -ltinfo ${CMAKE_CXX_STANDARD_LIBRARIES}")
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

