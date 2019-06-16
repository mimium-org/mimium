cmake_minimum_required(VERSION 3.4)

set(LLVM_CONFIG "/usr/local/bin/llvm-config")
# set(LLVM_DIR "/usr/local/opt/llvm/share/cmake/modules")
list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/llvm/")

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs support core irreader)



set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS "-Wall -g -O0")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-O3 -s")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17")
set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

project(dummy NONE)
  if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(CMAKE_CXX_COMPILER /usr/local/opt/llvm/bin/clang++)
    set(CMAKE_C_COMPILER /usr/local/opt/llvm/bin/clang)
  endif()
project (mimium CXX)

list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/flex)
list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/bison)
find_package(BISON 3.2 REQUIRED)
find_package(Flex REQUIRED)


include_directories(src ${FLEX_INCLUDE_DIRS} libs/rtaudio)


add_subdirectory( libs/rtaudio )

add_subdirectory( src )
add_subdirectory( test )
