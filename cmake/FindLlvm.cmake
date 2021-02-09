# Copyright (c) 2014 Andrew Kelley
# This file is MIT licensed.
# See http://opensource.org/licenses/MIT

# LLVM_FOUND
# LLVM_INCLUDE_DIRS
# LLVM_LIBRARIES
# LLVM_LIBDIRS
if(APPLE)
set(HOMEBREW_PATH /usr/local)
elseif(UNIX)
set(HOMEBREW_PATH /home/linuxbrew/.linuxbrew )
endif()
find_path(LLVM_INCLUDE_DIRS NAMES llvm/IR/IRBuilder.h
  PATHS
    ${HOMEBREW_PATH}/include
    ${HOMEBREW_PATH}/opt/llvm/include
    /usr/local/opt/llvm/include
    /usr/local/include
    /usr/include
    /mingw64/include
    C:/tools/msys64/mingw64/include
)
find_program(LLVM_CONFIG_EXE
      NAMES llvm-config llvm-config-11 llvm-config11
      PATHS
      ${HOMEBREW_PATH}/bin
      ${HOMEBREW_PATH}/opt/llvm/bin
      /usr/local/opt/llvm/bin
      /usr/local/bin
      /usr/bin
      "/mingw64/bin"
      "/c/msys64/mingw64/bin"
      "C:/tools/msys64/mingw64/bin"
      "C:/Libraries/llvm-11.0.0/bin"
      )

  if ("${LLVM_CONFIG_EXE}" STREQUAL "LLVM_CONFIG_EXE-NOTFOUND")
    message(FATAL_ERROR "unable to find llvm-config")
  endif()

  execute_process(
    COMMAND ${LLVM_CONFIG_EXE} --version
    OUTPUT_VARIABLE LLVM_CONFIG_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "llvm-config:  ${LLVM_CONFIG_EXE} ${LLVM_CONFIG_VERSION}")
  if("${LLVM_CONFIG_VERSION}" VERSION_LESS 9)
    message(FATAL_ERROR "expected LLVM 9.x~ but found ${LLVM_CONFIG_VERSION} using ${LLVM_CONFIG_EXE}")
  endif()

  execute_process(
    COMMAND ${LLVM_CONFIG_EXE} --targets-built
      OUTPUT_VARIABLE LLVM_TARGETS_BUILT_SPACES
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REPLACE " " ";" LLVM_TARGETS_BUILT "${LLVM_TARGETS_BUILT_SPACES}")
  function(NEED_TARGET TARGET_NAME)
      list (FIND LLVM_TARGETS_BUILT "${TARGET_NAME}" _index)
      if (${_index} EQUAL -1)
          message(FATAL_ERROR "LLVM (according to ${LLVM_CONFIG_EXE}) is missing target ${TARGET_NAME}. mimium requires LLVM to be built with all default targets enabled.")
      endif()
  endfunction(NEED_TARGET)
  # NEED_TARGET("AArch64")
  # NEED_TARGET("AMDGPU")
  NEED_TARGET("ARM")
#   NEED_TARGET("AVR")
  # NEED_TARGET("BPF")
  # NEED_TARGET("Hexagon")
  # NEED_TARGET("Lanai")
  # NEED_TARGET("Mips")
  # NEED_TARGET("MSP430")
  # NEED_TARGET("NVPTX")
  # NEED_TARGET("PowerPC")
  # NEED_TARGET("RISCV")
  # NEED_TARGET("Sparc")
  # NEED_TARGET("SystemZ")
  NEED_TARGET("WebAssembly")
  NEED_TARGET("X86")
  # NEED_TARGET("XCore")
  if((NOT LLVM_INCLUDE_DIRS))
  execute_process(
    COMMAND ${LLVM_CONFIG_EXE} --includedir
    OUTPUT_VARIABLE LLVM_INCLUDE_DIRS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
  
  if(NOT LLVM_LIBRARIES)
    execute_process(
        COMMAND ${LLVM_CONFIG_EXE} --libs --ignore-libllvm
        OUTPUT_VARIABLE LLVM_LIBRARIES_SPACES
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REPLACE " " ";" LLVM_LIBRARIES "${LLVM_LIBRARIES_SPACES}")
    
    execute_process(
        COMMAND ${LLVM_CONFIG_EXE} --system-libs
        OUTPUT_VARIABLE LLVM_SYSTEM_LIBS_SPACES
        OUTPUT_STRIP_TRAILING_WHITESPACE)
        string(REPLACE " " ";" LLVM_SYSTEM_LIBS_TMP "${LLVM_SYSTEM_LIBS_SPACES}")
        string(REPLACE "-llibxml2.tbd" "" LLVM_SYSTEM_LIBS "${LLVM_SYSTEM_LIBS_TMP}")
    execute_process(
        COMMAND ${LLVM_CONFIG_EXE} --libdir
        OUTPUT_VARIABLE LLVM_LIBDIRS_SPACES
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REPLACE " " ";" LLVM_LIBDIRS "${LLVM_LIBDIRS_SPACES}")
  endif()

  set(LLVM_LIBRARIES ${LLVM_LIBRARIES} ${LLVM_SYSTEM_LIBS})

  if(NOT LLVM_LIBRARIES)
    find_library(LLVM_LIBRARIES NAMES LLVM LLVM-11 LLVM-11.0)
  endif()

  link_directories("${CMAKE_PREFIX_PATH}/lib")
  link_directories("${LLVM_LIBDIRS}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(llvm DEFAULT_MSG LLVM_LIBRARIES LLVM_INCLUDE_DIRS)

mark_as_advanced(LLVM_INCLUDE_DIRS LLVM_LIBRARIES LLVM_LIBDIRS)