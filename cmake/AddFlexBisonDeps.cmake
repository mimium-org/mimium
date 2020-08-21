list(APPEND CMAKE_PREFIX_PATH  /usr/local /usr/local/opt/flex)
list(APPEND CMAKE_PREFIX_PATH  /usr/local /usr/local/opt/bison)
find_package(BISON 3.3 REQUIRED)
find_package(FLEX 2.6 REQUIRED)

BISON_TARGET(MyParser mimium.yy ${CMAKE_CURRENT_BINARY_DIR}/mimium_parser.cpp
DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/mimium_parser.hpp
VERBOSE REPORT_FILE ${CMAKE_CURRENT_BINARY_DIR}/bison.log
)
FLEX_TARGET(MyScanner mimium.l ${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp)

ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)
