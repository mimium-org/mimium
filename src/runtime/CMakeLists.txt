add_library(mimium_scheduler STATIC scheduler.cpp)
target_compile_features(mimium_scheduler PUBLIC cxx_std_17)
target_include_directories(mimium_scheduler 
PRIVATE
${MIMIUM_SOURCE_DIR}
)

target_link_libraries(mimium_scheduler PRIVATE 
mimium_utils)

add_subdirectory(backend)
add_subdirectory(JIT)