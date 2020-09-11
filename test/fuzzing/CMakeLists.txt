add_executable(fuzzing_compiler fuzzing_compiler.cpp)

target_compile_options(fuzzing_compiler PRIVATE -std=c++17 -fsanitize=fuzzer,address )
add_dependencies(fuzzing_compiler mimium_builtinfn)
target_include_directories(fuzzing_compiler
    PRIVATE
    .
    ${SNDFILE_INCLUDE_DIRS}
    ${FLEX_INCLUDE_CACHE}
    ${PARSER_HEADER_DIR}
    )
target_link_options(fuzzing_compiler PRIVATE -g -fsanitize=address,fuzzer)
target_link_libraries(fuzzing_compiler
    PRIVATE
      mimium_utils
      mimium_compiler
      mimium_runtime_jit
    )
file(COPY fuzz_dictionary.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/corpus)

add_custom_target(
    FuzzingCompilerTest
    COMMAND fuzzing_compiler -dict=fuzz_dictionary.txt -max_total_time=1800 -max_len=8192 -jobs=1 -only_ascii=1 -print_pcs=1 -runs=1000 ./corpus
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
# add_test(Name FuzzingCompiler 
# Command fuzzing_compiler  -print_final_stats=1 -dict=fuzz_dictionary.txt -max_total_time=1800 -jobs=8 ./corpus)
  