#pragma once
#include <array>
#include <cstring>
#include <optional>
#include <string>

#include "indexed-paths.hpp"
#include "sort.hpp"

auto find_viewable_directory(const char* path, bool skip_current = false) -> std::optional<std::string>;
auto get_sorted_images(const char* path) -> IndexedPaths;
auto get_sorted_directories(const char* path) -> IndexedPaths;
auto get_next_image_file(const char* path, bool reverse) -> std::optional<IndexedPaths>;
auto get_next_directory(const char* path, bool reverse, const char* root = nullptr) -> std::optional<std::string>;

namespace {
template <class T, class E>
auto contains(const T& vec, const E& elm) -> bool {
    return std::find(vec.begin(), vec.end(), elm) != vec.end();
}

inline auto is_image_file(const char* const path) -> bool {
    constexpr auto extensions = std::array{".jpg", ".jpeg", ".png", ".gif", ".webp", ".bmp", ".avif"};
    if(!std::filesystem::is_regular_file(path)) {
        return false;
    }
    const auto extension = Path(path).extension().string();
    return contains(extensions, extension);
}

inline auto list_files(const char* const path) -> IndexedPaths {
    auto files = IndexedPaths(path);
    for(const std::filesystem::directory_entry& it : std::filesystem::directory_iterator(path)) {
        files.append(it.path().filename().string());
    }
    files.sort();
    return files;
}

inline auto get_next_file(const IndexedPaths& files, const char* const current, const bool reverse) -> std::optional<IndexedPaths> {
    if(auto p = std::find(files.begin(), files.end(), Path(current).filename().string()); p == files.end() || p == (reverse ? files.begin() : files.end() - 1)) {
        return std::nullopt;
    } else {
        auto new_files = files;
        new_files.set_index(std::distance(files.begin(), p) + (reverse ? -1 : 1));
        return new_files;
    }
}

inline auto get_next_directory_recursive(const char* const path, const bool reverse, const bool skip_current, const char* const root) -> std::optional<std::string> {
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
    // check parent directories
    auto parent = Path(path).parent_path().string();
    if(root != nullptr && std::strcmp(parent.data(), root) == 0) {
        return std::nullopt;
    }
    return get_next_directory_recursive(parent.data(), reverse, true, root);
}
} // namespace

inline auto find_viewable_directory(const char* const path, const bool skip_current) -> std::optional<std::string> {
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

inline auto get_sorted_images(const char* const path) -> IndexedPaths {
    auto files = list_files(path);
    files.filter([](const std::string& path) {
        return !is_image_file(path.data());
    });
    return files;
}

inline auto get_sorted_directories(const char* const path) -> IndexedPaths {
    auto files = list_files(path);
    files.filter([](const std::string& path) {
        return !std::filesystem::is_directory(path.data());
    });
    return files;
}

inline auto get_next_image_file(const char* const path, const bool reverse) -> std::optional<IndexedPaths> {
    const auto images = get_sorted_images(Path(path).parent_path().string().data());
    return get_next_file(images, path, reverse);
}

inline auto get_next_directory(const char* const path, const bool reverse, const char* const root) -> std::optional<std::string> {
    return get_next_directory_recursive(path, reverse, false, root);
}

enum class PathCheckResult {
    Exists,
    Retrieved,
    Fatal,
};

inline auto check_and_retrieve_paths(const char* const root, IndexedPaths& files, const bool reverse) -> PathCheckResult {
    if(std::filesystem::exists(files.get_current())) {
        return PathCheckResult::Exists;
    }
    auto images  = true;
    auto missing = Path(files.get_current());
    auto path    = missing.parent_path();
    while(1) {
        do {
            if(!std::filesystem::is_directory(path)) {
                break;
            }
            auto updated = images ? get_sorted_images(path.string().data()) : get_sorted_directories(path.string().data());
            if(updated.empty()) {
                break;
            }
            const auto missing_name = missing.filename().string();
            auto       file_names   = updated.get_file_names();
            file_names.emplace_back(missing_name);
            sort_strings(file_names);
            auto       index            = static_cast<size_t>(std::distance(file_names.begin(), std::find(file_names.begin(), file_names.end(), missing_name)));
            const auto selectable_front = (index != (file_names.size() - 1));
            const auto selectable_back  = (index != 0);
            file_names.erase(file_names.begin() + index);
            index += (reverse && selectable_back) ? -1 : (!reverse && selectable_front) ? 0
                                                     : selectable_front                 ? -1
                                                                                        : 0;
            const auto paths = IndexedPaths(path.string(), file_names, index);
            if(images) {
                files = std::move(paths);
            } else {
                auto viewable = find_viewable_directory(paths[index].data());
                if(!viewable) {
                    break;
                }
                files = get_sorted_images(viewable->data());
            }
            return PathCheckResult::Retrieved;
        } while(0);
        if(path == root) {
            return PathCheckResult::Fatal;
        }
        images  = false;
        missing = std::move(path);
        path    = missing.parent_path();
    }
}
