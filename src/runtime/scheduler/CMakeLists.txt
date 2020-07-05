add_library(mimium_scheduler SHARED scheduler.cpp)
target_compile_options(mimium_scheduler PUBLIC -std=c++17)
target_include_directories(mimium_scheduler PRIVATE)

target_link_libraries(mimium_scheduler mimium_utils)

install(TARGETS mimium_scheduler DESTINATION lib)
