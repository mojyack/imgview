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
    void               sort();
    void               filter(const std::function<bool(const std::string&)>& pred);
    void               append(const std::string& path);
    std::string        get_full_path(const char* filename) const;
    size_t             get_index() const;
    bool               set_index(const size_t index);
    bool               set_index_by_name(const char* filename);
    std::string        get_current() const;
    const std::string& get_base() const;
    const Files&       get_file_names() const;
    size_t             size() const;
    bool               empty() const;
    std::string        operator[](const size_t index) const;
    Iterator           begin();
    Iterator           end();
    ConstIterator      begin() const;
    ConstIterator      end() const;
    IndexedPaths(){};
    IndexedPaths(const std::string& base);
    IndexedPaths(std::string&& base, Files& files, const size_t index);
};
