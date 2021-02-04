/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "filereader.hpp"
#include <cassert>
#include <fstream>
#include <unordered_map>
#include <utility>

#include "error_def.hpp"
#include "helper_functions.hpp"
namespace {
const std::unordered_map<std::string_view, mimium::FileType> str_to_filetype = {
    {mimium::mmm_ext, mimium::FileType::MimiumSource},
    {mimium::ll_ext, mimium::FileType::LLVMIR},
    {mimium::bc_ext, mimium::FileType::LLVMIR},
};
};

namespace mimium {

FileType getFileTypeByExt(std::string_view ext) { return getEnumByStr(str_to_filetype, ext); }

std::pair<fs::path, FileType> getFilePath(std::string_view val) {
  fs::path res(val);
  auto type = getFileTypeByExt(res.extension().string());
  if (type == FileType::Invalid) {
    throw std::runtime_error("Unknown file type. Expected either of .mmm, .ll or .bc");
  }
  return std::pair(res, type);
}

FileReader::FileReader(fs::path cwd) : cwd(std::move(cwd)) {}
Source FileReader::loadFile(std::string const& path) {
  fs::current_path(cwd);
  auto [srcpath, type] = getFilePath(path);
  Source res{fs::absolute(srcpath), type, ""};

  // memo: fs::exists(path,ec) for .mmm file returns file type of "unknown", not "regular" or
  // "none". to prevent error, need to check specifically not to be "not found"
  std::error_code ec;
  auto status = fs::status(res.filepath, ec);
  if (status.type() == fs::file_type::none || status.type() == fs::file_type::not_found) {
    throw FileNotFound(res.filepath.string());
  }

  if (res.filetype == FileType::Invalid) { throw UnknownExtension(res.filepath.string()); }

  auto ifs = std::make_unique<std::ifstream>();
  ifs->open(res.filepath.string());
  assert(*ifs && "ifs should not fail");
  res.source = std::string(std::istreambuf_iterator<char>(*ifs), std::istreambuf_iterator<char>());
  return res;
}
}  // namespace mimium