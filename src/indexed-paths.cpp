#include "indexed-paths.hpp"
#include "sort.hpp"

auto IndexedPaths::sort() -> void {
    sort_string(files);
}
auto IndexedPaths::filter(const std::function<bool(const std::string&)>& pred) -> void {
    std::erase_if(files, [this, &pred](const std::string& path) {
        std::string full = base + "/" + path;
        return pred(full);
    });
}
auto IndexedPaths::append(const std::string& path) -> void {
    files.emplace_back(path);
}
auto IndexedPaths::get_full_path(const char* filename) const -> std::string {
    return base + "/" + filename;
}
auto IndexedPaths::get_index() const -> size_t {
    return index;
}
auto IndexedPaths::set_index(const size_t i) -> bool {
    if(i < files.size()) {
        index = i;
        return true;
    }
    return false;
}
auto IndexedPaths::set_index_by_name(const char* const filename) -> bool {
    if(auto p = std::find(files.begin(), files.end(), filename); p != files.end()) {
        index = std::distance(files.begin(), p);
        return true;
    }
    return false;
}
auto IndexedPaths::get_current() const -> std::string {
    return this->operator[](index);
}
auto IndexedPaths::get_base() const -> const std::string& {
    return base;
}
auto IndexedPaths::get_file_names() const -> const IndexedPaths::Files& {
    return files;
}
auto IndexedPaths::size() const -> size_t {
    return files.size();
}
auto IndexedPaths::empty() const -> bool {
    return files.empty();
}
auto IndexedPaths::operator[](const size_t index) const -> std::string {
    return get_full_path(files[index].data());
}
auto IndexedPaths::begin() -> IndexedPaths::Iterator {
    return files.begin();
}
auto IndexedPaths::end() -> IndexedPaths::Iterator {
    return files.end();
}
auto IndexedPaths::begin() const -> IndexedPaths::ConstIterator {
    return files.cbegin();
}
auto IndexedPaths::end() const -> IndexedPaths::ConstIterator {
    return files.cend();
}
IndexedPaths::IndexedPaths(const std::string& base) : base(base) {}
IndexedPaths::IndexedPaths(std::string&& base, Files& files, const size_t index) : base(base), files(std::move(files)), index(index) {}
