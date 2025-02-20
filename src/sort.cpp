#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

auto sort_strings(std::vector<std::string>& strings) -> void {
    auto swap_flag = std::unordered_set<size_t>();
    auto max_len   = 0uz;
    for(auto& s : strings) {
        if(auto len = s.size(); len > max_len) {
            max_len = len;
        }
    }
    for(auto i = 0uz; i < max_len; i += 1) {
        const auto k = max_len - i;

        auto sort_count = 0uz;
        for(auto j = 0uz; j < strings.size(); j += 1) {
            if(strings[j].size() >= k) {
                swap_flag.emplace(j);
                sort_count += 1;
            }
        }
        auto sep = strings.size() - sort_count;
        for(auto j = 0uz, swapped = 0uz; j < strings.size(); j += 1) {
            if(swap_flag.contains(j) && j < sep) {
                if(!swap_flag.contains(sep + swapped)) {
                    std::iter_swap(strings.begin() + j, strings.begin() + sep + swapped);
                } else {
                    while(swap_flag.contains(sep + swapped)) {
                        swapped += 1;
                        if(sep + swapped >= strings.size()) {
                            break;
                        } else {
                            std::iter_swap(strings.begin() + j, strings.begin() + sep + swapped);
                        }
                    }
                }
                swap_flag.erase(j);
                swap_flag.erase(sep + swapped);
                swapped += 1;
            }
        }

        auto copy_buffer    = std::vector<std::string>(sort_count);
        auto char_and_index = std::vector<std::pair<char, size_t>>(sort_count);

        for(auto j = 0uz; j < sort_count; j += 1) {
            copy_buffer[j] = std::move(strings[sep + j]);
            if(auto c = copy_buffer[j][k - 1]; c >= 65 && c <= 90) {
                char_and_index[j].first = c + 32;
            } else {
                char_and_index[j].first = c;
            }
            char_and_index[j].second = j;
        }
        std::stable_sort(char_and_index.begin(), char_and_index.end());
        for(auto j = 0uz; j < sort_count; j += 1) {
            strings[sep + j] = std::move(copy_buffer[char_and_index[j].second]);
        }
    }
}
