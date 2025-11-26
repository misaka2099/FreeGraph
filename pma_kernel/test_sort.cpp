#include <iostream>
#include <thread>
int main()
{
    int edge_info[16] = {5,6,8,0,7,9,10,14,13,15,2,1,11,12,4,3};
    // for (int i = 0 ; i < 16 ; i++)
    // {
    //     edge_info[i] = i;
    // }
    // for (int stage = 0; stage < 4; stage++) { // log2(16) = 4
    //         for (int step = 0; step < 16; step++) {
    //             #pragma HLS UNROLL
    //             int idx1 = step;
    //             int idx2 = step ^ (1 << stage);
    //             if (idx2 > idx1 && idx2 < 16) {
    //                 if (edge_info[idx1] > edge_info[idx2]) {
    //                     int temp = edge_info[idx1];
    //                     edge_info[idx1] = edge_info[idx2];
    //                     edge_info[idx2] = temp;
    //                 }
    //             }
    //         }
    //     }
        // for (int i = 1; i < 16; i++) {
        //     #pragma HLS UNROLL
        //     auto key = edge_info[i];
        //     int j = i - 1;
        //     while (j >= 0 && edge_info[j] > key) {
        //         edge_info[j + 1] = edge_info[j];
        //         j--;
        //     }
        //     edge_info[j + 1] = key;
        // }
    
    
    for (int i = 0 ; i < 16 ; i++)
    {
        std::cout << edge_info[i] << " ";
    }

    std::cout << std::endl;
    return 0;
}