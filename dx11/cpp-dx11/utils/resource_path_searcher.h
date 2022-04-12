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
  fs::path find_path(fs::path filename) const;
};
} // namespace PD
