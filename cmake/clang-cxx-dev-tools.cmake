# derived from http://www.labri.fr/perso/fleury/posts/programming/using-clang-tidy-and-clang-format.html
# Additional targets to perform clang-format/clang-tidy
# Get all project files
file(GLOB_RECURSE
     ALL_CXX_SOURCE_FILES
     *.[chi]pp *.[chi]xx *.cc *.hh *.ii *.[CHI]
     )

# Adding clang-format target if executable is found
find_program(CLANG_FORMAT "clang-format")
if(CLANG_FORMAT)
  add_custom_target(
    clang-format
    COMMAND /usr/local/bin/clang-format
    -i
    -style=file
    ${ALL_CXX_SOURCE_FILES}
    )
endif()

# Adding clang-tidy target if executable is found
find_program(CLANG_TIDY "clang-tidy")
if(CLANG_TIDY)
  add_custom_target(
    clang-tidy
    COMMAND /usr/local/opt/llvm/bin/clang-tidy
    ${ALL_CXX_SOURCE_FILES}
    -config=''
    --
    -std=c++17
    ${INCLUDE_DIRECTORIES}
    )
endif()