# Found on http://hg.kvats.net
#
# - Try to find libsndfile
# 
# Once done this will define
#
#  SNDFILE_FOUND - system has libsndfile
#  SNDFILE_INCLUDE_DIRS - the libsndfile include directory
#  SNDFILE_LIBRARIES - Link these to use libsndfile
#
#  Copyright (C) 2006  Wengo
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

if (SNDFILE_LIBRARIES AND SNDFILE_INCLUDE_DIRS)
# in cache already
set(SNDFILE_FOUND TRUE)
else (SNDFILE_LIBRARIES AND SNDFILE_INCLUDE_DIRS)
if(APPLE)
set(HOMEBREW_PATH /usr/local)
elseif(UNIX)
set(HOMEBREW_PATH /home/linuxbrew/.linuxbrew )
else()#windows
set(CMAKE_FIND_LIBRARY_PREFIXES "lib")  
endif()
find_path(SNDFILE_INCLUDE_DIR
  REQUIRED
    NAMES
      sndfile.h
    PATHS
      ${HOMEBREW_PATH}/include
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
      C:/tools/msys64/mingw64/include
  )
  
  find_library(SNDFILE_LIBRARY
    NAMES
      libsndfile.a sndfile 
    PATHS
      ${HOMEBREW_PATH}/lib
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
      /mingw64/lib
      C:/tools/msys64/mingw64/lib
  )

  find_library(OGG_LIBRARY
  NAMES
  libogg.a ogg
  PATHS
  ${HOMEBREW_PATH}/lib
  /usr/lib
  /usr/lib/x86_64-linux-gnu/
  /usr/local/lib
  /opt/local/lib
  /sw/lib
  C:/tools/msys64/mingw64/lib
  )
  find_library(VORBIS_LIBRARY
  NAMES
  libvorbis.a vorbis 
  PATHS
  ${HOMEBREW_PATH}/lib
  /usr/lib
  /usr/lib/x86_64-linux-gnu/
  /usr/local/lib
  /opt/local/lib
  /sw/lib
  C:/tools/msys64/mingw64/lib
  )
  find_library(VORBISENC_LIBRARY
  NAMES
  libvorbisenc.a vorbisenc
  PATHS
  ${HOMEBREW_PATH}/lib
  /usr/lib
  /usr/lib/x86_64-linux-gnu/
  /usr/local/lib
  /opt/local/lib
  /sw/lib
  C:/tools/msys64/mingw64/lib
  )
  find_library(FLAC_LIBRARY
  NAMES
  libFLAC.a flac FLAC
  PATHS
  ${HOMEBREW_PATH}/lib
  /usr/lib
  /usr/lib/x86_64-linux-gnu/
  /usr/local/lib
  /opt/local/lib
  /sw/lib
  C:/tools/msys64/mingw64/lib
  )
  find_library(OPUS_LIBRARY  REQUIRED
  NAMES
  libopus.a opus
  PATHS
  ${HOMEBREW_PATH}/lib
  /usr/lib
  /usr/lib/x86_64-linux-gnu/
  /usr/local/lib
  /opt/local/lib
  /sw/lib
  C:/tools/msys64/mingw64/lib
  /mingw64/lib

  )
  # find_package(OPUS REQUIRED)
  set(SNDFILE_INCLUDE_DIRS
    ${SNDFILE_INCLUDE_DIR}
  )
  set(SNDFILE_LIBRARIES
  ${SNDFILE_LIBRARY}
  ${FLAC_LIBRARY}
  ${VORBIS_LIBRARY}
  ${VORBISENC_LIBRARY}
  ${OGG_LIBRARY}
  ${OPUS_LIBRARY}
  )
  message(STATUS "------------------")

message(STATUS ${SNDFILE_INCLUDE_DIR})
message(STATUS     ${SNDFILE_LIBRARY})

  if (SNDFILE_INCLUDE_DIR AND SNDFILE_LIBRARY)
    set(SNDFILE_FOUND TRUE)
  endif (SNDFILE_INCLUDE_DIR AND SNDFILE_LIBRARY)

  if (SNDFILE_FOUND)
    if (NOT SndFile_FIND_QUIETLY)
      message(STATUS "Found libsndfile: ${SNDFILE_LIBRARIES}")
    endif (NOT SndFile_FIND_QUIETLY)
  else (SNDFILE_FOUND)
    if (SndFile_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find libsndfile")
    endif (SndFile_FIND_REQUIRED)
  endif (SNDFILE_FOUND)

  # show the SNDFILE_INCLUDE_DIRS and SNDFILE_LIBRARIES variables only in the advanced view
  mark_as_advanced(SNDFILE_INCLUDE_DIRS SNDFILE_LIBRARIES)

endif (SNDFILE_LIBRARIES AND SNDFILE_INCLUDE_DIRS)