add_library(mimium_backend STATIC audiodriver.cpp)

target_compile_options(mimium_backend PUBLIC -std=c++17)

target_include_directories(mimium_backend PUBLIC)

target_link_libraries(mimium_backend mimium_utils mimium_scheduler)

add_subdirectory(rtaudio)


