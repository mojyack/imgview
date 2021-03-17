#include <array>
#include <cstring>

#include "path.hpp"
#include "sort.hpp"

namespace {
template <typename T, typename E>
bool contains(const T& vec, const E& elm) {
    return std::find(vec.begin(), vec.end(), elm) != vec.end();
}
bool is_image_file(const char* path) {
    constexpr std::array EXTENSIONS = {".jpg", ".jpeg", ".png", ".gif", ".webp", ".bmp"};
    if(!std::filesystem::is_regular_file(path)) {
        return false;
    }
    const auto extension = Path(path).extension().string();
    return contains(EXTENSIONS, extension);
}
IndexedPaths list_files(const char* path) {
    IndexedPaths files(path);
    for(const std::filesystem::directory_entry& it : std::filesystem::directory_iterator(path)) {
        files.append(it.path().filename().string());
    }
    files.sort();
    return files;
}
std::optional<IndexedPaths> get_next_file(const IndexedPaths& files, const char* current, const bool reverse) {
    if(auto p = std::find(files.begin(), files.end(), Path(current).filename().string()); p == files.end() || p == (reverse ? files.begin() : files.end() - 1)) {
        return std::nullopt;
    } else {
        auto new_files = files;
        new_files.set_index(std::distance(files.begin(), p) + (reverse ? -1 : 1));
        return new_files;
    }
}
std::optional<std::string> get_next_directory_recursive(const char* path, const bool reverse, const bool skip_current, const char* root) {
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
    const auto  directories = get_sorted_directories(Path(path).parent_path().string().data());
    auto        next        = get_next_file(directories, path, reverse);
    std::string viewable;
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

std::optional<std::string> find_viewable_directory(const char* path, const bool skip_current) {
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
IndexedPaths get_sorted_images(const char* path) {
    auto files = list_files(path);
    files.filter([](const std::string& path) {
        return !is_image_file(path.data());
    });
    return files;
}
IndexedPaths get_sorted_directories(const char* path) {
    auto files = list_files(path);
    files.filter([](const std::string& path) {
        return !std::filesystem::is_directory(path.data());
    });
    return files;
}
std::optional<IndexedPaths> get_next_image_file(const char* path, const bool reverse) {
    const auto images = get_sorted_images(Path(path).parent_path().string().data());
    return get_next_file(images, path, reverse);
}
std::optional<std::string> get_next_directory(const char* path, const bool reverse, const char* root) {
    return get_next_directory_recursive(path, reverse, false, root);
}
