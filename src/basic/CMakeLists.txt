add_library(mimium_filereader filereader.cpp)
target_compile_features(mimium_filereader PUBLIC cxx_std_17)
target_include_directories(mimium_filereader 
INTERFACE
$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mimium>
PRIVATE
$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>)
target_compile_options(mimium_filereader PRIVATE -fvisibility=hidden)


add_library(mimium_utils STATIC mir.cpp type.cpp ast.cpp ast_to_string.cpp)
target_include_directories(mimium_utils 
INTERFACE
$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mimium>
PRIVATE
$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>)
target_compile_features(mimium_utils PUBLIC cxx_std_17)

target_compile_options(mimium_utils PUBLIC
$<$<CONFIG:Debug>:-O0 -DMIMIUM_DEBUG_BUILD -Wall -pedantic-errors>
-fvisibility=hidden
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

# install(TARGETS mimium_utils DESTINATION lib)