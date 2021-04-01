#pragma once
#include <optional>
#include <string>

#include "indexed-paths.hpp"

bool                        is_layer_file(const char* path);
std::optional<std::string>  find_viewable_directory(const char* path, const bool skip_current = false);
IndexedPaths                get_sorted_images(const char* path);
IndexedPaths                get_sorted_directories(const char* path);
std::optional<IndexedPaths> get_next_image_file(const char* path, const bool reverse);
std::optional<std::string>  get_next_directory(const char* path, const bool reverse, const char* root = nullptr);
