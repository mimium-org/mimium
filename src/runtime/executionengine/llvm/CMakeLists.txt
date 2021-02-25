add_library(mimium_llvm_jitengine STATIC llvm_jitengine.cpp)

target_compile_options(mimium_llvm_jitengine PUBLIC -std=c++17)
add_dependencies(mimium_llvm_jitengine mimium_utils)
target_include_directories(mimium_llvm_jitengine
INTERFACE
$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mimium>
PRIVATE
$<BUILD_INTERFACE:${LLVM_INCLUDE_DIRS}>
$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
)
target_compile_options(mimium_llvm_jitengine PRIVATE
${LLVM_CXX_FLAGS})

target_link_libraries(mimium_llvm_jitengine 
PUBLIC 
PRIVATE
$<BUILD_INTERFACE:${LLVM_LIBRARIES}>
mimium_runtime
)
target_link_options(mimium_llvm_jitengine PRIVATE
${LLVM_LD_FLAGS})