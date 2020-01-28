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
  #prevent from updating everytime - for offline environment
  UPDATE_COMMAND ""
  ### Add cmake args 
  CONFIGURE_COMMAND
  "${CMAKE_COMMAND}"
  "-H${CMAKE_BINARY_DIR}/externals/rtaudio"
  "-B${CMAKE_BINARY_DIR}/externals/rtaudio/build"
  INSTALL_COMMAND ""
  ### Path to build tree
  BUILD_COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/externals/rtaudio/build
)

add_dependencies(mimium_backend_rtaudio rtaudio_ext)

target_compile_options(mimium_backend_rtaudio PUBLIC -std=c++17)

target_include_directories(mimium_backend_rtaudio PUBLIC ${SNDFILE_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/externals/rtaudio)

target_link_directories(mimium_backend_rtaudio PUBLIC ${CMAKE_BINARY_DIR}/externals/rtaudio/build)

target_link_libraries(mimium_backend_rtaudio rtaudio)


