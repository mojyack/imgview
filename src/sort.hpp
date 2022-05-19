#pragma once
#include <string>
#include <unordered_map>
#include <vector>

inline auto sort_string(std::vector<std::string>& strings) -> void {
    auto swap_flag = std::unordered_map<size_t, bool>();
    auto max_len   = size_t(0);
    for(auto& s : strings) {
        if(auto len = s.size(); len > max_len) {
            max_len = len;
        }
    }
    for(auto i = size_t(0); i < max_len; i += 1) {
        const auto k = max_len - i;

        auto sort_count = size_t(0);
        for(auto j = size_t(0); j < strings.size(); j += 1) {
            if(strings[j].size() >= k) {
                swap_flag[j] = true;
                sort_count += 1;
            }
        }
        auto sep = strings.size() - sort_count;
        for(auto j = size_t(0), swapped = size_t(0); j < strings.size(); j += 1) {
            if(swap_flag[j] && j < sep) {
                if(!swap_flag[sep + swapped]) {
                    std::iter_swap(strings.begin() + j, strings.begin() + sep + swapped);
                } else {
                    while(swap_flag[sep + swapped]) {
                        swapped += 1;
                        if(sep + swapped >= strings.size()) {
                            break;
                        } else {
                            std::iter_swap(strings.begin() + j, strings.begin() + sep + swapped);
                        }
                    }
                }
                swap_flag[j]             = false;
                swap_flag[sep + swapped] = false;
                swapped += 1;
            }
        }

        auto copy_buffer    = std::vector<std::string>(sort_count);
        auto char_and_index = std::vector<std::pair<char, size_t>>(sort_count);

        for(auto j = size_t(0); j < sort_count; j += 1) {
            copy_buffer[j] = strings[sep + j];
            if(auto c = copy_buffer[j][k - 1]; c >= 65 && c <= 90) {
                char_and_index[j].first = c + 32;
            } else {
                char_and_index[j].first = c;
            }
            char_and_index[j].second = j;
        }
        std::stable_sort(char_and_index.begin(), char_and_index.end());
        for(auto j = size_t(0); j < sort_count; j += 1) {
            strings[sep + j] = copy_buffer[char_and_index[j].second];
        }
    }
}
