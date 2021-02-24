/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <sstream>
#include <system_error>
namespace mimium {

// base class of all error.

struct Error : public std::runtime_error {
  explicit Error(const std::string& what) : std::runtime_error(what){};
};

// related to compiler

struct CompileError : public Error {
  explicit CompileError(const std::string& what) : Error(what){};
};

struct PreprocessorError : public CompileError {
  explicit PreprocessorError(const std::string& what) : CompileError(what){};
};

struct FileNotFound : public PreprocessorError {
  explicit FileNotFound(const std::string& what) : PreprocessorError("File not found: " + what){};
};
struct UnknownExtension : public PreprocessorError {
  explicit UnknownExtension(const std::string& what)
      : PreprocessorError("Unknown File Extension. Use .mmm Instead: " + what){};
};

// related to compiler

struct RuntimeError : public Error {
    explicit RuntimeError(const std::string& what) : Error(what){};

};
}  // namespace mimium