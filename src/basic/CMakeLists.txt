add_library(mimium_filereader STATIC filereader.cpp)
target_compile_features(mimium_filereader PUBLIC cxx_std_17)



add_library(mimium_utils STATIC mir.cpp type.cpp ast_to_string.cpp)
target_include_directories(mimium_utils PUBLIC  ${MIMIUM_SOURCE_DIR})
target_compile_features(mimium_utils PUBLIC cxx_std_17)

target_compile_options(mimium_utils PUBLIC
$<$<CONFIG:Debug>:-O0 -DMIMIUM_DEBUG_BUILD -Wall -pedantic-errors>
)
if((CMAKE_BUILD_TYPE STREQUAL "Debug") AND 
    (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
message("sanitizer activated")
target_compile_options(mimium_utils PUBLIC 
   -fsanitize=address -fno-omit-frame-pointer
)
target_link_options(mimium_utils PUBLIC 
    -fsanitize=address
)
endif()

install(TARGETS mimium_utils DESTINATION lib)