

# ----start rtaudio config
message(STATUS "Subproject: Rtaudio...")

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/rtaudio.CMakeLists.txt
  ${CMAKE_BINARY_DIR}/rtaudio-download/CMakeLists.txt)
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
message(STATUS "Subproject: RtAudio...DONE")
# execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" ${CMAKE_BINARY_DIR}/rtaudio-src
#   RESULT_VARIABLE result
#   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/rtaudio-build)
# add_subdirectory(${CMAKE_BINARY_DIR}/rtaudio-src ${CMAKE_BINARY_DIR}/rtaudio-build)

# import
find_package(RtAudio REQUIRED)

set(RTAUDIO_INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR}/rtaudio-src)

add_library(mimium_backend_rtaudio SHARED driver_rtaudio.cpp)

target_include_directories(mimium_backend_rtaudio 
PUBLIC
${RTAUDIO_INCLUDE_DIRECTORIES}
${MIMIUM_SOURCE_DIR}
)
target_compile_options(mimium_backend_rtaudio PRIVATE 
-std=c++17)

target_compile_definitions(mimium_backend_rtaudio PRIVATE
	)

target_link_libraries(mimium_backend_rtaudio PUBLIC 
RtAudio::rtaudio
mimium_scheduler
${RTAUDIO_INTERFACE_LIBS}
)
install( TARGETS mimium_backend_rtaudio DESTINATION lib)

