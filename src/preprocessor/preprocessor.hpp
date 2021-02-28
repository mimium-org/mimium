/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <list>
#include <unordered_set>
#include "basic/filereader.hpp"

namespace mimium {

class Preprocessor {
 public:
  explicit Preprocessor(fs::path cwd);
  Source process(fs::path path);

 private:
  static Source loadFile(const fs::path& path, const fs::path& base_path);
  static std::list<std::string> splitSource(const std::string& str);
  void replaceIncludeMacro(std::list<std::string>& src, const fs::path& base_path);
  std::unordered_set<std::string> files;
  fs::path cwd;
};

}  // namespace mimium