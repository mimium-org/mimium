add_library(mimium_scheduler STATIC scheduler.cpp)
target_compile_options(mimium_scheduler PUBLIC -std=c++17)
target_include_directories(mimium_scheduler 
PUBLIC
${MIMIUM_SOURCE_DIR}
)

target_link_libraries(mimium_scheduler mimium_utils)

add_subdirectory(backend)
add_subdirectory(JIT)