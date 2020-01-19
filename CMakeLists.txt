cmake_minimum_required(VERSION 3.4)

 
set(CMAKE_CXX_COMPILER /usr/local/opt/llvm/bin/clang++)
set(CMAKE_C_COMPILER /usr/local/opt/llvm/bin/clang) 

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
PROJECT(mimium CXX)

set(LLVM_CONFIG "/usr/local/Cellar/llvm/9.0.0debug/bin/llvm-config")

# set(LLVM_DIR "/usr/local/opt/llvm/share/cmake/modules")
list(APPEND CMAKE_PREFIX_PATH "/usr/local/Cellar/llvm/9.0.0debug")

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})



set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Og -g -DNDEBUG")

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})


# for macOS SDK Root

list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/flex)
list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/bison)
find_package(BISON 3.2 REQUIRED VERBOSE REPORT_FILE ${CMAKE_CURRENT_BINARY_DIR}/bison.log)
find_package(Flex REQUIRED)


# Including custom cmake rules for clang-tidy and clang-format
include(cmake/clang-cxx-dev-tools.cmake)

set(RTAUDIO_BUILD_TESTING FALSE)
add_subdirectory( libs/rtaudio )

set(RTMIDI_BUILD_TESTING FALSE)
set(RTMIDI_TARGETNAME_UNINSTALL "uninstall_rtmidi" CACHE STRING "Name of 'uninstall' build target")
add_subdirectory( libs/rtmidi )



find_package(SndFile REQUIRED)

include_directories(src ${FLEX_INCLUDE_DIRS} libs/rtaudio libs/rtmidi ${SNDFILE_INCLUDE_DIRS})


add_subdirectory( src )

add_subdirectory( docs )

#add_subdirectory( test )
