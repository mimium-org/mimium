/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "preprocessor.hpp"
#include <numeric>
#include <regex>
#include <sstream>
#include <utility>
namespace mimium {

Preprocessor::Preprocessor(fs::path cwd) : cwd(std::move(cwd)) {}

Source Preprocessor::loadFile(fs::path path) {
  FileReader filereader(cwd);
  return filereader.loadFile(path);
}

std::list<std::string> Preprocessor::splitSource(const std::string& str) {
  std::list<std::string> res;
  std::stringstream ss(str);
  std::string tmpstr;
  while (std::getline(ss, tmpstr, '\n')) {
    if (!tmpstr.empty()) { res.emplace_back(tmpstr); }
  }
  return res;
}
void Preprocessor::replaceIncludeMacro(std::list<std::string>& src) {
  const std::regex re(R"((include)(\s)+\"(.*)\")");
  for (auto&& iter = src.begin(); iter != src.cend(); /*increment manually*/) {
    const auto& line = *iter;
    std::smatch matchres;
    std::regex_match(line, matchres, re);
    if (!matchres.empty()) {
      const auto& filename = matchres[3];
      fs::path newfilepath(filename.str());
      std::list<std::string> newsrclist;
      if (files.count(newfilepath) == 0) {
        auto new_src = this->loadFile(newfilepath);
        newsrclist = splitSource(new_src.source);
      }
      auto nextiter = std::next(iter);
      auto insertpoint = src.erase(iter);
      src.splice(insertpoint, std::move(newsrclist));
      iter = nextiter;
    } else {
      iter++;
    }
  }
}
Source Preprocessor::process(fs::path path) {
  auto src = loadFile(path);
  files.emplace(src.filepath.string());
  auto src_list_by_line = splitSource(src.source);
  replaceIncludeMacro(src_list_by_line);
  std::string res;
  src.source =
      std::accumulate(src_list_by_line.begin(), src_list_by_line.end(), res,
                      [&](std::string& acc, std::string& line) { return acc + "\n" + line; });
  return src;
}

}  // namespace mimium