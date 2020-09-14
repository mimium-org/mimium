cmake_minimum_required(VERSION 3.8)
set(MIMIUM_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(basic)
add_subdirectory(compiler)
add_subdirectory(runtime)

add_executable(mimium main.cpp)
add_dependencies(default_build mimium)
add_executable(mimium_llloader llloader.cpp)
add_dependencies(default_build mimium_llloader)

target_compile_options(mimium PUBLIC -std=c++17)
target_compile_options(mimium_llloader PUBLIC -std=c++17)
target_compile_definitions(mimium PUBLIC MIMIUM_VERSION=\"v${CMAKE_PROJECT_VERSION}\")
target_compile_definitions(mimium_llloader PUBLIC MIMIUM_VERSION=\"v${CMAKE_PROJECT_VERSION}\")

target_include_directories(mimium
PRIVATE
# ${CMAKE_SOURCE_DIR}/libs/rtmidi
# ${SNDFILE_INCLUDE_DIRS}
${FLEX_INCLUDE_CACHE}
${PARSER_HEADER_DIR} 

)
target_include_directories(mimium_llloader
PRIVATE
# ${CMAKE_SOURCE_DIR}/libs/rtmidi
# ${SNDFILE_INCLUDE_DIRS}
${FLEX_INCLUDE_CACHE}
${PARSER_HEADER_DIR} 
)

target_link_libraries(mimium
    LLVMCore
    mimium_compiler
    mimium_runtime_jit
    mimium_backend_rtaudio
    mimium_builtinfn 
    )
target_link_libraries(mimium_llloader
    mimium_compiler
    mimium_runtime_jit
    mimium_backend_rtaudio
    mimium_builtinfn 
    )


install (TARGETS mimium mimium_llloader 
        EXPORT  mimium-export
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(EXPORT mimium-export
    FILE mimium-config.cmake
    DESTINATION share/cmake/mimium/
    EXPORT_LINK_INTERFACE_LIBRARIES) 
install(FILES "${CMAKE_SOURCE_DIR}/README.md" "${CMAKE_SOURCE_DIR}/LICENSE.md" "${CMAKE_SOURCE_DIR}/COPYRIGHT" "${CMAKE_SOURCE_DIR}/mimium_logo_slant.svg"
    DESTINATION .)

    set(CPACK_GENERATOR "TGZ;ZIP")
    set(CPACK_PACKAGE_VENDOR "mimium development community(Original Author: Tomoya Matsuura)")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "mimium - MInimal Musical MedIUM, an infrastractural programming language for sound and music.")
    set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
    set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
    set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
    set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/mimium_logo_slant.svg")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.md")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
    # to make github action naming easier
    set(CPACK_PACKAGE_FILE_NAME "mimium-v${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
include(CPack)


add_custom_target(uninstall)
add_custom_command(TARGET uninstall
COMMAND cat ${CMAKE_BINARY_DIR}/install_manifest.txt | sudo xargs rm -f
)
