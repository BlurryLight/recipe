#include "resource_path_searcher.h"
#include "cmake_vars.h"
#include <iostream>
#include <spdlog/spdlog.h>
using namespace PD;
fs::path ResourcePathSearcher::root_path; // ROOT_DIR is defined by CMake,
                                          // which is the project root dir
ResourcePathSearcher::ResourcePathSearcher() {
  root_path = Path(ROOT_DIR);
  search_paths_.emplace(root_path);
}

void ResourcePathSearcher::add_path(const std::string &path) {
  Path p(path);
  if (!p.is_absolute())
    p = fs::absolute(p);
  search_paths_.emplace(p);
}

fs::path ResourcePathSearcher::find_path(fs::path filename) const {
  for (const Path &p : search_paths_) {
    if (!fs::exists(p))
      continue;
    auto path = p / filename;
    if (fs::exists(path)) {
      spdlog::debug("Resource: {} found!", path.u8string());
      // It may cause problems in Windows because Windows native path is encoded
      // in UTF16LE To Handle this problem will need complex machanism like
      // writing wstring overloads for all related functions.I won't bother to
      // do that.
      return path.is_absolute() ? path : fs::absolute(path);
    }
  }
  std::string msg = "ResourcePathSearch cannot find " + filename.u8string();
  spdlog::error(msg);
  spdlog::shutdown();
  std::terminate();
}

void ResourcePathSearcher::add_path(const ResourcePathSearcher::Path &path) {
  auto p = fs::absolute(path);
  search_paths_.emplace(p);
}
