/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once
#ifndef MIMIUM_VERSION
#define MIMIUM_VERSION "unspecified"
#endif

#include "basic/type.hpp"
#include "basic/ast.hpp"
#include "basic/ast_to_string.hpp"
#include "basic/mir.hpp"

#include "preprocessor/preprocessor.hpp"

#include "compiler/compiler.hpp"
#include "compiler/ffi.hpp"

#include "runtime/backend/rtaudio/driver_rtaudio.hpp"
#include "runtime/executionengine/llvm/llvm_jitengine.hpp"

#include "frontend/genericapp.hpp"
#include "frontend/cli.hpp"