#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct FileList {
    std::filesystem::path    prefix;
    std::vector<std::string> files;
    size_t                   index;
};

auto get_parent_dir(std::string_view dir) -> std::string;
auto list_files(std::string_view dir) -> std::optional<FileList>;
auto filter_non_image_files(FileList& list) -> void;
auto filter_regular_files(FileList& list) -> void;
