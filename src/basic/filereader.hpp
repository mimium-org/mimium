/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <filesystem>
#include <fstream>
#include "export.hpp"
namespace mimium {
namespace fs = std::filesystem;

constexpr std::string_view mmm_ext = ".mmm";
constexpr std::string_view ll_ext = ".ll";
constexpr std::string_view bc_ext = ".bc";

enum class FileType {
  Invalid = -1,
  MimiumSource = 0,
  MimiumMir,  // currently not used
  LLVMIR,
};
struct MIMIUM_DLL_PUBLIC Source {
  fs::path filepath;
  FileType filetype;
  std::string source;
};
MIMIUM_DLL_PUBLIC  FileType getFileTypeByExt(std::string_view ext);
MIMIUM_DLL_PUBLIC  std::pair<fs::path, FileType> getFilePath(std::string_view val);

class MIMIUM_DLL_PUBLIC FileReader {
 public:
  explicit FileReader(fs::path cwd);
  Source loadFile(std::string const& path);

 private:
  fs::path cwd;
  inline const static std::string_view mimium_extensiton = ".mmm";
};
}  // namespace mimium