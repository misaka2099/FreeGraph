#include <iostream>

void sub_search(
    int* cache_bs_helper,
    int cache_size, //num of int
    int src,
    int dst,
    bool work,
    int &searched_seg_number
)
{
    if (!work) {
        searched_seg_number = -1;
        return;
    }
    int low = 0;
    int high = cache_size - 1;
    searched_seg_number = -1;

    while (low <= high) {
        int mid = (low + high) / 2;
        if (cache_bs_helper[mid] == src) {
            searched_seg_number = mid;
            return;
        } else if (cache_bs_helper[mid] < src) {
            low = mid + 1;
            searched_seg_number = mid; // Record the last position where cache_bs_helper[mid] < src
        } else {
            high = mid - 1;
        }
    }
}
int main()
{
    int cache_bs_helper[] = {-1};
    int searched_seg_number;
    for (int i = 0; i < 21; i++) {
        sub_search(cache_bs_helper, 1, i, 0, true, searched_seg_number);
        std::cout << "Searched segment number for " << i << ": " << searched_seg_number << std::endl;
    }
    return 0;
}   