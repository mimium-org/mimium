# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/. 

# Original Copyright http://hg.kvats.net
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

function(setlibname  output name_val)
  if(UNIX AND (NOT APPLE))
    set(output ${name_val})
  else()
    set(output lib${name_val}.a)
  endif()
endfunction()
function(find_library_private libname)
string(TOUPPER ${libname} libname_u)
set(LIBNAMEPRIVATE ${libname_u}_LIBNAME)
setlibname(${libname_u}_LIBNAME libname)
find_library(${libname_u}_LIBRARY REQUIRED
NAMES
  ${LIBNAMEPRIVATE}
PATHS
  ${HOMEBREW_PATH}/lib
  /usr/lib
  /usr/local/lib
  /opt/local/lib
  /sw/lib
  /mingw64/lib
  C:/tools/msys64/mingw64/lib
)
endfunction()

find_library_private(sndfile)
find_library_private(ogg)
find_library_private(vorbis)
find_library_private(vorbisenc)
find_library_private(FLAC)
find_library_private(opus)

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
message(STATUS ${SNDFILE_LIBRARY})

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