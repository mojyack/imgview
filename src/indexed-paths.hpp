#pragma once
#include <filesystem>
#include <functional>
#include <vector>

using Path = std::filesystem::path;
class IndexedPaths {
  private:
    using Files         = std::vector<std::string>;
    using Iterator      = Files::iterator;
    using ConstIterator = Files::const_iterator;

    std::string base;
    Files       files;
    size_t      index = 0;

  public:
    auto sort() -> void;
    auto filter(const std::function<bool(const std::string&)>& pred) -> void;
    auto append(const std::string& path) -> void;
    auto get_full_path(const char* filename) const -> std::string;
    auto get_index() const -> size_t;
    auto set_index(size_t index) -> bool;
    auto set_index_by_name(const char* filename) -> bool;
    auto get_current() const -> std::string;
    auto get_base() const -> const std::string&;
    auto get_file_names() const -> const Files&;
    auto size() const -> size_t;
    auto empty() const -> bool;
    auto operator[](size_t index) const -> std::string;
    auto begin() -> Iterator;
    auto end() -> Iterator;
    auto begin() const -> ConstIterator;
    auto end() const -> ConstIterator;
    IndexedPaths(){};
    IndexedPaths(const std::string& base);
    IndexedPaths(std::string&& base, Files& files, size_t index);
};
