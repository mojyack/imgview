#include <array>
#include <cstring>

#include "path.hpp"
#include "sort.hpp"

namespace {
template <typename T, typename E>
auto contains(const T& vec, const E& elm) -> bool {
    return std::find(vec.begin(), vec.end(), elm) != vec.end();
}
auto is_image_file(const char* const path) -> bool {
    constexpr auto EXTENSIONS = std::array{".jpg", ".jpeg", ".png", ".gif", ".webp", ".bmp"};
    if(!std::filesystem::is_regular_file(path)) {
        return false;
    }
    const auto extension = Path(path).extension().string();
    return contains(EXTENSIONS, extension);
}
auto list_files(const char* const path) -> IndexedPaths {
    auto files = IndexedPaths(path);
    for(const std::filesystem::directory_entry& it : std::filesystem::directory_iterator(path)) {
        files.append(it.path().filename().string());
    }
    files.sort();
    return files;
}
auto get_next_file(const IndexedPaths& files, const char* const current, const bool reverse) -> std::optional<IndexedPaths> {
    if(auto p = std::find(files.begin(), files.end(), Path(current).filename().string()); p == files.end() || p == (reverse ? files.begin() : files.end() - 1)) {
        return std::nullopt;
    } else {
        auto new_files = files;
        new_files.set_index(std::distance(files.begin(), p) + (reverse ? -1 : 1));
        return new_files;
    }
}
auto get_next_directory_recursive(const char* const path, const bool reverse, const bool skip_current, const char* const root) -> std::optional<std::string> {
    if(std::strcmp(path, "/") == 0) {
        return std::nullopt;
    }
    if(!skip_current) {
        // check sub directories.
        if(auto r = find_viewable_directory(path, true)) {
            return r.value();
        }
    }

    // check directories in same level.
    const auto directories = get_sorted_directories(Path(path).parent_path().string().data());
    auto       next        = get_next_file(directories, path, reverse);
    auto       viewable    = std::string();
    while(next.has_value()) {
        if(auto r = find_viewable_directory(next->get_current().data())) {
            viewable = r.value();
            break;
        }
        next = get_next_file(directories, next->get_current().data(), reverse);
    }
    if(next.has_value()) {
        // found in same level.
        return viewable;
    }
    // check parent directory.
    auto parent = Path(path).parent_path().string();
    if(root != nullptr && std::strcmp(parent.data(), root) == 0) {
        return std::nullopt;
    }
    return get_next_directory_recursive(parent.data(), reverse, true, root);
}
} // namespace

auto find_viewable_directory(const char* const path, const bool skip_current) -> std::optional<std::string> {
    const auto files = list_files(path);
    if(!skip_current) {
        for(const auto& p : files) {
            if(is_image_file(files.get_full_path(p.data()).data())) {
                return path;
            }
        }
    }
    for(const auto& p : files) {
        const auto full_path = files.get_full_path(p.data());
        if(!std::filesystem::is_directory(full_path.data())) {
            continue;
        }
        if(auto r = find_viewable_directory(full_path.data()); r.has_value()) {
            return r.value();
        }
    }
    return std::nullopt;
}
auto get_sorted_images(const char* const path) -> IndexedPaths {
    auto files = list_files(path);
    files.filter([](const std::string& path) {
        return !is_image_file(path.data());
    });
    return files;
}
auto get_sorted_directories(const char* const path) -> IndexedPaths {
    auto files = list_files(path);
    files.filter([](const std::string& path) {
        return !std::filesystem::is_directory(path.data());
    });
    return files;
}
auto get_next_image_file(const char* const path, const bool reverse) -> std::optional<IndexedPaths> {
    const auto images = get_sorted_images(Path(path).parent_path().string().data());
    return get_next_file(images, path, reverse);
}
auto get_next_directory(const char* const path, const bool reverse, const char* const root) -> std::optional<std::string> {
    return get_next_directory_recursive(path, reverse, false, root);
}
