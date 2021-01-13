/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <filesystem>

namespace mimium {
namespace fs = std::filesystem;

struct Source {
  fs::path filepath;
  std::string source;
};

class FileReader {
 public:
  explicit FileReader(fs::path cwd);
  Source loadFile(std::string const& path);
private:
fs::path cwd;
  inline const static std::string_view mimium_extensiton = ".mmm";

};
}  // namespace mimium