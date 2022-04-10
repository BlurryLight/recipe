#pragma once

#include <filesystem>
namespace fs = std::filesystem;

#include <set>
#include <string>
#include <vector>
namespace PD {

class ResourcePathSearcher {
public:
  using Path = std::filesystem::path;

  std::set<Path> search_paths_;

public:
  static Path root_path; // project root path
  ResourcePathSearcher();
  ResourcePathSearcher(const ResourcePathSearcher &) = delete;
  ResourcePathSearcher &operator=(const ResourcePathSearcher &) = delete;
  ResourcePathSearcher(ResourcePathSearcher &&) = delete;
  void add_path(const std::string &path);
  void add_path(const Path &paths);
  // pass a vector such as {"src","cores","xxx.cc"}
  fs::path find_path(const std::vector<std::string> &paths) const;
  fs::path find_path(const std::string &filename) const;
};
} // namespace PD
