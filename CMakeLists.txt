cmake_minimum_required(VERSION 3.4)

set(LLVM_CONFIG "/usr/local/bin/llvm-config")
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS "-Wall")
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



#add_subdirectory( CppCmb )

add_executable(Main main.cpp)
