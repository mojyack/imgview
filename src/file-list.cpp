#include <filesystem>

#include "file-list.hpp"
#include "sort.hpp"

namespace {
template <class T, class E>
auto contains(const T& vec, const E& elm) -> bool {
    return std::find(vec.begin(), vec.end(), elm) != vec.end();
}

auto is_image_file(const std::filesystem::path path) -> bool {
    constexpr auto extensions = std::array{".jpg", ".jpeg", ".png", ".jxl", ".gif", ".webp", ".bmp", ".avif", ".txt"};
    if(!std::filesystem::is_regular_file(path)) {
        return false;
    }
    return contains(extensions, path.extension().string());
}
} // namespace

auto get_parent_dir(std::string_view dir) -> std::string {
    return std::filesystem::path(dir).parent_path().string();
}

auto list_files(const std::string_view path) -> FileList {
    auto fl = FileList{std::string(path), {}, 0};
    for(const auto& it : std::filesystem::directory_iterator(path)) {
        fl.files.push_back(it.path().filename().string());
    }
    sort_strings(fl.files);
    return fl;
}

auto filter_non_image_files(FileList& list) -> void {
    std::erase_if(list.files, [&list](const std::string& path) {
        return !is_image_file(list.prefix / path);
    });
    return;
}
