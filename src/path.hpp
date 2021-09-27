#pragma once
#include <optional>
#include <string>

#include "indexed-paths.hpp"

auto find_viewable_directory(const char* path, bool skip_current = false) -> std::optional<std::string>;
auto get_sorted_images(const char* path) -> IndexedPaths;
auto get_sorted_directories(const char* path) -> IndexedPaths;
auto get_next_image_file(const char* path, bool reverse) -> std::optional<IndexedPaths>;
auto get_next_directory(const char* path, bool reverse, const char* root = nullptr) -> std::optional<std::string>;
