find_package(PkgConfig)

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
  "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/externals/rtaudio/build"
  "-H${CMAKE_BINARY_DIR}/externals/rtaudio"
  "-B${CMAKE_BINARY_DIR}/externals/rtaudio/build"
  "-DRTAUDIO_BUILD_STATIC_LIBS=TRUE"
  INSTALL_COMMAND sh -c "${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/externals/rtaudio/build/cmake_install.cmake"
  ### Path to build tree
  BUILD_COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/externals/rtaudio/build
)

add_dependencies(mimium_backend_rtaudio rtaudio_ext)

pkg_check_modules(RTAUDIO REQUIRED ${CMAKE_BINARY_DIR}/externals/rtaudio/build/rtaudio.pc)

message(${RTAUDIO_FOUND})
message(${RTAUDIO_INCLUDE_DIRS})
message(${RTAUDIO_PREFIX})
message(${RTAUDIO_LIBRARIES})
message(${RTAUDIO_CFLAGS})

target_compile_options(mimium_backend_rtaudio PRIVATE -std=c++17)
if (${APPLE})
  find_library(COREAUDIO_LIB CoreAudio)
  find_library(COREFOUNDATION_LIB CoreFoundation)
  # list(APPEND LINKLIBS ${COREAUDIO_LIB} ${COREFOUNDATION_LIB})
endif()
target_include_directories(mimium_backend_rtaudio PRIVATE ${RTAUDIO_INCLUDE_DIRS})
target_compile_options(mimium_backend_rtaudio INTERFACE ${RTAUDIO_CFLAGS})
target_link_directories(mimium_backend_rtaudio INTERFACE ${RTAUDIO_LIBRARY_DIRS})
target_link_libraries(mimium_backend_rtaudio INTERFACE ${RTAUDIO_LIBRARIES} ${COREAUDIO_LIB} ${COREFOUNDATION_LIB})


