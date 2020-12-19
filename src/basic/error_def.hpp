/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <sstream>
#include <system_error>
namespace mimium {

// base class of all error.

struct Error : public std::runtime_error {
  explicit Error(const char* what) : std::runtime_error(what){};
};

// related to compiler

struct CompileError : public Error {
  explicit CompileError(const char* what) : Error(what){};
};

struct PreprocessorError : public CompileError {
  explicit PreprocessorError(const char* what) : CompileError(what){};
};

struct FileNotFound : public PreprocessorError {
  explicit FileNotFound(const char* what)
      : PreprocessorError(("File not found: " + std::string(what)).c_str()){};
};
struct UnknownExtension : public PreprocessorError {
  explicit UnknownExtension(const char* what)
      : PreprocessorError(
            ("Unknown File Extension. Use .mmm Instead: " + std::string(what)).c_str()){};
};

// related to compiler

struct RuntimeError : public Error {};
}  // namespace mimium