#include "data_type.h"
#include <iostream>
#include "pma.h"
int main()
{
    int data[SEGMENT_SIZE * 128];
    PMA_HEADER header;
    for (int i = 0 ; i < VERTEX_PER_PMA ; i++)
    {
        header.vertex_range[i] = 2*i+2;
    }
    for (int i = 0 ; i < SEGMENT_SIZE * 128 ; i++)
    {
        data[i] = UNUSED_ICON;
    }
    for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
    {
        for (int i = 0 ; i < SEGMENT_SIZE / 2 ; i++)
        {
            int src = v;
            int dst = i * 100;
            insert_edge(src, dst, data, header);
        }
    }
    for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
    {
        for (int i = v == 0 ? 0 : header.vertex_range[v - 1] * SEGMENT_SIZE; i < header.vertex_range[v] * SEGMENT_SIZE; i++)
        {
            std::cout << data[i] << " ";
        }
        std::cout << std::endl;
    }

    rebalance(data, header);
    std::cout << "after rebalance" << std::endl;
    for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
    {
        for (int i = v == 0 ? 0 : header.vertex_range[v - 1] * SEGMENT_SIZE; i < header.vertex_range[v] * SEGMENT_SIZE; i++)
        {
            std::cout << data[i] << " ";
        }
        std::cout << std::endl;
    }
    return 0;
}