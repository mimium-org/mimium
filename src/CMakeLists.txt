cmake_minimum_required(VERSION 3.8)
# set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 1)

add_subdirectory(basic)
add_subdirectory(preprocessor)
add_subdirectory(compiler)
add_subdirectory(runtime)

add_library(mimium libmimium.cpp)
target_compile_options(mimium PUBLIC -std=c++17)
target_compile_definitions(mimium PUBLIC MIMIUM_VERSION=\"${CMAKE_PROJECT_VERSION}\")
target_include_directories(mimium
INTERFACE
$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mimium>
PRIVATE
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_compile_options(mimium PUBLIC
-fvisibility=hidden
)
target_link_options(mimium PUBLIC
)
target_link_libraries(mimium
    PUBLIC
    mimium_filereader
    mimium_preprocessor
    mimium_compiler
    mimium_llvm_jitengine 
    mimium_backend_rtaudio
    mimium_builtinfn
    mimium_utils
    )

add_subdirectory(frontend)
include(GenerateExportHeader)
add_executable(mimium_exe main.cpp)
set_target_properties(mimium_exe PROPERTIES ENABLE_EXPORTS ON)

add_dependencies(default_build mimium)
set_target_properties(mimium_exe PROPERTIES OUTPUT_NAME mimium)
    
target_link_libraries(mimium_exe PRIVATE mimium_cli)
target_include_directories(mimium_exe
PRIVATE
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

install (TARGETS
            mimium_utils
            mimium_filereader
            mimium_parser
            mimium_preprocessor
            mimium_compiler
            mimium_llvm_codegen
            mimium_llvm_jitengine
            mimium_runtime
            mimium_scheduler
            mimium_audiodriver
            mimium_backend_rtaudio
            mimium_builtinfn 
            mimium_genericapp mimium_cli 
            mimium mimium_exe
        EXPORT  mimium-export
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/ # source directory
        DESTINATION "include/mimium" # target directory
        FILES_MATCHING # install only matched files
        PATTERN "*.hpp" # select header files
)

install(EXPORT mimium-export
    FILE mimium-config.cmake
    NAMESPACE mimium::
    DESTINATION share/cmake/mimium/
    EXPORT_LINK_INTERFACE_LIBRARIES) 

set(CPACK_GENERATOR "TGZ;ZIP")
set(CPACK_PACKAGE_VENDOR "mimium development community(Original Author: Tomoya Matsuura)")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "mimium - MInimal Musical MedIUM, an infrastractural programming language for sound and music.")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
# The options are not worked for Archive Generator
set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/mimium_logo_slant.svg")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.md")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

# used in CPackAdditionalInstall.cmake
set(CPACK_ADDITIONAL_INSTALL_RESOURCES     
"${CMAKE_SOURCE_DIR}/README.md"
"${CMAKE_SOURCE_DIR}/CHANGELOG.md"
"${CMAKE_SOURCE_DIR}/LICENSE.md"
"${CMAKE_SOURCE_DIR}/COPYRIGHT"
"${CMAKE_SOURCE_DIR}/mimium_logo_slant.svg") 
set(CPACK_INSTALL_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/CPackAdditionalInstall.cmake")

# to make github action naming easier
set(CPACK_PACKAGE_FILE_NAME "mimium-v${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
include(CPack)


add_custom_target(uninstall)
add_custom_command(TARGET uninstall
COMMAND cat ${CMAKE_BINARY_DIR}/install_manifest.txt | sudo xargs rm -f
)
