#include <ap_int.h>
#include <iostream>
using namespace std;
int main()
{
    int arr[10] = {0,1,2,3,5,6,7,8,9,-1};
    ap_uint<320> to_op;
    for (int i = 0; i < 10; i++) {
        to_op.range((i+1)*32 - 1, i*32) = arr[i];
    }

    int to_insert_pos = -1;
    ap_uint<320> mask = ((ap_uint<320>) 1 << ((to_insert_pos + 1)*32)) - 1;
    auto upper = to_op;
    upper &= ~mask;
    upper <<= 32;
    to_op &= mask;
    to_op |= upper;
    to_op.range((to_insert_pos + 1) * 32 + 31, (to_insert_pos + 1) * 32) = 4;
    for (int i = 0; i < 10; i++) {
        cout << to_op.range((i+1)*32 - 1, i*32).to_int() << ' ';
    }

    return 0;
}