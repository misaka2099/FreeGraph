#include <iostream>
static inline bool edge_less(
    const int &a,
    const int &b)
{
    return a < b;
}

static inline void compare_swap(
    int &a,
    int &b,
    bool dir)
{
#pragma HLS INLINE

    bool swap_needed = dir ? edge_less(b, a) : edge_less(a, b);

    if (swap_needed) {
        int tmp = a;
        a = b;
        b = tmp;
    }
}

int main()
{
    int edge_info[16]= {16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1};
    BITONIC_K:
    for (int k = 2; k <= 16; k <<= 1) {

    BITONIC_J:
            for (int j = k >> 1; j > 0; j >>= 1) {

    COMPARE_STAGE:
                for (int i = 0; i < 16; i++) {

                int ixj = i ^ j;

                if (ixj > i) {
                    bool dir = ((i & k) == 0); // ascending half
                    compare_swap(edge_info[i], edge_info[ixj], dir);
                }
            }
        }
    }
    for (int i = 0; i < 16; i++)
    {
        std::cout << edge_info[i] << " ";
    }
    std::cout << std::endl;
}
     