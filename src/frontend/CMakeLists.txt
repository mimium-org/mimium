add_library(mimium_genericapp STATIC genericapp.cpp)
target_link_libraries(mimium_genericapp PUBLIC mimium)
target_compile_features(mimium_genericapp PUBLIC cxx_std_17)

add_library(mimium_cli STATIC cli.cpp)
target_link_libraries(mimium_cli PUBLIC mimium_genericapp)

