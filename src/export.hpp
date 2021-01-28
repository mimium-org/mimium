/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#if defined _WIN32 || defined __CYGWIN__
  #if defined(MIMIUM_EXPORT)
    #define MIMIUM_DLL_PUBLIC __declspec(dllexport)
  #else
    #define MIMIUM_DLL_PUBLIC
  #endif
#else
  #if __GNUC__ >= 4
    #define MIMIUM_DLL_PUBLIC __attribute__( (visibility( "default" )) )
  #else
    #define MIMIUM_DLL_PUBLIC
  #endif
#endif