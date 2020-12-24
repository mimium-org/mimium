/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "filereader.hpp"
#include "basic/error_def.hpp"

#include <cassert>
#include <fstream>
#include <utility>
namespace mimium {

FileReader::FileReader(fs::path cwd) : cwd(std::move(cwd)) {}
Source FileReader::loadFile(std::string path) {
  fs::current_path(cwd);
  Source res;
  auto abspath = fs::absolute(res.filepath);
  res.filepath = abspath;

  // memo: fs::exists(path,ec) for .mmm file returns file type of "unknown", not "regular" or
  // "none". to prevent error, need to check specifically not to be "not found"
  std::error_code ec;
  auto status = fs::status(abspath, ec);
  if (status.type() == fs::file_type::none || status.type() == fs::file_type::not_found) {
    throw FileNotFound(abspath.string());
  }

  if (abspath.extension() == mimium_extensiton) { throw UnknownExtension(abspath.string()); }

  std::ifstream ifs(abspath.string());
  assert(ifs && "ifs should not fail");
  res.source = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
  return res;
}
}  // namespace mimium