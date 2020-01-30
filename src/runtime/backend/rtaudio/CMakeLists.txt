

# ----start rtaudio config
message(STATUS "Subproject: Rtaudio...")

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/rtaudio.CMakeLists.txt
  ${CMAKE_BINARY_DIR}/rtaudio-download/CMakeLists.txt)
set(RTAUDIO_TARGETNAME_UNINSTALL "uninstall_mimium")
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/rtaudio-download)
if(result)
  message(FATAL_ERROR "CMake step for rtaudio failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/rtaudio-download)
if(result)
  message(FATAL_ERROR "Build step for rtaudio failed: ${result}")
endif()

add_subdirectory(
	${CMAKE_BINARY_DIR}/rtaudio-src
	${CMAKE_BINARY_DIR}/rtaudio-build)

message(STATUS "Subproject: RtAudio...DONE")
execute_process(COMMAND ls 
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/rtaudio-build)

find_package(RtAudio REQUIRED)

add_library(mimium_backend_rtaudio SHARED driver_rtaudio.cpp)

set(RTAUDIO_INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR}/rtaudio-src)
get_target_property(RTAUDIO_INTERFACE_LIBS RtAudio::rtaudio INTERFACE_LINK_LIBRARIES)

target_include_directories(mimium_backend_rtaudio PUBLIC
${RTAUDIO_INCLUDE_DIRECTORIES}
)
target_compile_options(mimium_backend_rtaudio PRIVATE 
-std=c++17)

target_compile_definitions(mimium_backend_rtaudio PRIVATE
	)

target_link_libraries(mimium_backend_rtaudio PUBLIC 
rtaudio
mimium_scheduler
${RTAUDIO_INTERFACE_LIBS}
)


