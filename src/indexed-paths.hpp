#pragma once
#include <filesystem>
#include <functional>
#include <vector>

#include "sort.hpp"

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
    auto sort() -> void {
        sort_string(files);
    }

    auto filter(const std::function<bool(const std::string&)> pred) -> void {
        std::erase_if(files, [this, &pred](const std::string& path) {
            std::string full = base + "/" + path;
            return pred(full);
        });
    }

    auto append(const std::string& path) -> void {
        files.emplace_back(path);
    }

    auto get_full_path(const char* const filename) const -> std::string {
        return base + "/" + filename;
    }

    auto get_index() const -> size_t {
        return index;
    }

    auto set_index(const size_t new_index) -> bool {
        if(new_index < files.size()) {
            index = new_index;
            return true;
        }
        return false;
    }

    auto set_index_by_name(const char* const filename) -> bool {
        if(auto p = std::find(files.begin(), files.end(), filename); p != files.end()) {
            index = std::distance(files.begin(), p);
            return true;
        }
        return false;
    }

    auto get_current() const -> std::string {
        return (*this)[index];
    }

    auto get_base() const -> const std::string& {
        return base;
    }

    auto get_file_names() const -> const Files& {
        return files;
    }

    auto size() const -> size_t {
        return files.size();
    }

    auto empty() const -> bool {
        return files.empty();
    }

    auto operator[](const size_t index) const -> std::string {
        return get_full_path(files[index].data());
    }

    auto begin() -> IndexedPaths::Iterator {
        return files.begin();
    }

    auto end() -> IndexedPaths::Iterator {
        return files.end();
    }

    auto begin() const -> IndexedPaths::ConstIterator {
        return files.cbegin();
    }

    auto end() const -> IndexedPaths::ConstIterator {
        return files.cend();
    }

    IndexedPaths() = default;

    IndexedPaths(std::string base) : base(std::move(base)){};

    IndexedPaths(std::string base, Files files, const size_t index) : base(std::move(base)), files(std::move(files)), index(index){};
};
