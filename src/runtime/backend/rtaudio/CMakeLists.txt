add_library(mimium_backend_rtaudio STATIC driver_rtaudio.cpp)

set(RTAUDIO_BUILD_TESTING FALSE)

include(ExternalProject)
ExternalProject_Add(
  ### Target name
  rtaudio_ext
  ### Path to Git repository
  GIT_REPOSITORY https://github.com/thestk/rtaudio
  ### Specify TAG
  GIT_TAG master
  ### Path to source tree
  SOURCE_DIR ${CMAKE_BINARY_DIR}/externals/rtaudio
  ### Specify Generator
  CMAKE_GENERATOR "Linux Makefiles"
  ### Add cmake args 
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/externals/rtaudio/build -DCMAKE_BUILD_TYPE=Release
  ### Path to build tree
  BINARY_DIR ${CMAKE_BINARY_DIR}/externals/rtaudio/build
  BUILD_COMMAND make install
)

# add_subdirectory( ${CMAKE_SOURCE_DIR}/libs/rtaudio [EXCLUDE_FROM_ALL])

add_dependencies(mimium_backend_rtaudio rtaudio_ext)

target_compile_options(mimium_backend_rtaudio PUBLIC -std=c++17)

target_include_directories(mimium_backend_rtaudio PUBLIC ${SNDFILE_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/externals/rtaudio)

target_link_libraries(mimium_backend_rtaudio mimium_backend rtaudio)


