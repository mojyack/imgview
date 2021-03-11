#include "indexed-paths.hpp"
#include "sort.hpp"

void IndexedPaths::sort() {
    sort_string(files);
}
void IndexedPaths::filter(const std::function<bool(const std::string&)>& pred) {
    std::erase_if(files, [this, &pred](const std::string& path) {
        std::string full = base + "/" + path;
        return pred(full);
    });
}
void IndexedPaths::append(const std::string& path) {
    files.emplace_back(path);
}
std::string IndexedPaths::get_full_path(const char* filename) const {
    return base + "/" + filename;
}
size_t IndexedPaths::get_index() const {
    return index;
}
bool IndexedPaths::set_index(const size_t i) {
    if(i < files.size()) {
        index = i;
        return true;
    }
    return false;
}
bool IndexedPaths::set_index_by_name(const char* filename) {
    if(auto p = std::find(files.begin(), files.end(), filename); p != files.end()) {
        index = std::distance(files.begin(), p);
        return true;
    }
    return false;
}
std::string IndexedPaths::get_current() const {
    return this->operator[](index);
}
const std::string& IndexedPaths::get_base() const {
    return base;
}
const IndexedPaths::Files& IndexedPaths::get_file_names() const {
    return files;
}
size_t IndexedPaths::size() const {
    return files.size();
}
bool IndexedPaths::empty() const {
    return files.empty();
}
std::string IndexedPaths::operator[](const size_t index) const {
    return get_full_path(files[index].data());
}
IndexedPaths::Iterator IndexedPaths::begin() {
    return files.begin();
}
IndexedPaths::Iterator IndexedPaths::end() {
    return files.end();
}
IndexedPaths::ConstIterator IndexedPaths::begin() const {
    return files.cbegin();
}
IndexedPaths::ConstIterator IndexedPaths::end() const {
    return files.cend();
}
IndexedPaths::IndexedPaths(const std::string& base) : base(base) {}
IndexedPaths::IndexedPaths(std::string&& base, Files& files, const size_t index) : base(base), files(std::move(files)), index(index) {}
