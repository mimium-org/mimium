cmake_minimum_required(VERSION 3.4)
project(mimium_builtin_functions CXX)



add_library(builtin_functions  builtin_functions.cpp)

target_include_directories(builtin_functions
  PUBLIC ${PROJECT_SOURCE_DIR}
  )