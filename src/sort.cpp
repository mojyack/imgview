#include <map>

#include "sort.hpp"

void sort_string(std::vector<std::string>& strings) {
    std::map<int, bool> swap_flag;
    size_t              max_len = 0;
    for(auto& s : strings) {
        if(auto len = s.size(); len > max_len) max_len = len;
    }
    for(size_t i = 0; i < max_len; i++) {
        size_t k          = max_len - i;
        size_t sort_count = 0;
        for(size_t j = 0; j < strings.size(); j++) {
            if(strings[j].size() >= k) {
                swap_flag[j] = true;
                sort_count++;
            }
        }
        size_t sep = strings.size() - sort_count;
        for(size_t j = 0, swapped = 0; j < strings.size(); j++) {
            if(swap_flag[j] && j < sep) {
                if(!swap_flag[sep + swapped]) {
                    std::iter_swap(strings.begin() + j, strings.begin() + sep + swapped);
                } else {
                    while(swap_flag[sep + swapped]) {
                        swapped++;
                        if(sep + swapped >= strings.size()) {
                            break;
                        } else {
                            std::iter_swap(strings.begin() + j, strings.begin() + sep + swapped);
                        }
                    }
                }
                swap_flag[j]             = false;
                swap_flag[sep + swapped] = false;
                swapped++;
            }
        }

        std::vector<std::string>             copy_buffer(sort_count);
        std::vector<std::pair<char, size_t>> char_and_index(sort_count);

        for(size_t j = 0; j < sort_count; j++) {
            copy_buffer[j] = strings[sep + j];
            if(char c = copy_buffer[j][k - 1]; c >= 65 && c <= 90) {
                char_and_index[j].first = c + 32;
            } else {
                char_and_index[j].first = c;
            }
            char_and_index[j].second = j;
        }
        std::stable_sort(char_and_index.begin(), char_and_index.end());
        for(size_t j = 0; j < sort_count; j++) {
            strings[sep + j] = copy_buffer[char_and_index[j].second];
        }
    }
}
