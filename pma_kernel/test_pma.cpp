#include "data_type.h"
#include <iostream>
#include <vector>
#include "debug.h"
#include "pma.h"


void show_header(PMA_HEADER& header)
{
    printf("PMA HEADER:\n");
    for (int i = 0 ; i < VERTEX_PER_PMA ; i++)
    {
        printf("VERTEX %d: RANGE %d, EDGE COUNT %d, ATTR %f\n", i, header.vertex_range[i], header.edge_count[i], header.attr[i]);
    }
}
void show_pma(int* pma_data, PMA_HEADER& pma_header)
{
    printf("PMA DATA:\n");
    for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
    {
        if (pma_header.vertex_range[v] == UNUSED_ICON)
        {
            break;
        }
        int start = (v == 0) ? 0 : pma_header.vertex_range[v - 1] * SEGMENT_SIZE;
        int end = pma_header.vertex_range[v] * SEGMENT_SIZE;
        printf("Vertex %d edges: ", v);
        for (int i = start ; i < end ; i++)
        {
            if (pma_data[i] != UNUSED_ICON)
            {
                printf("%d ", pma_data[i]);
            }
        }
        printf("\n");
    }
}
void show_raw_pma(int* pma_data, PMA_HEADER& pma_header)
{
    printf("PMA DATA RAW:\n");
    for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
    {
        if (pma_header.vertex_range[v] == UNUSED_ICON)
        {
            break;
        }
        int start = (v == 0) ? 0 : pma_header.vertex_range[v - 1] * SEGMENT_SIZE;
        int end = pma_header.vertex_range[v] * SEGMENT_SIZE;
        printf("Vertex %d edges: ", v);
        for (int i = start ; i < end ; i++)
        {
            printf("%d ", pma_data[i]);
        }
        printf("\n");
    }
}


int main()
{
    int data[SEGMENT_SIZE * 128];
    memset(data, -1, sizeof(int) * SEGMENT_SIZE * 128);
    PMA_HEADER header;
    std::vector<int > src;
    std::vector<int > dst;
    for (int i = 0 ; i < 16 ; i++)
    {
        for (int j = 0 ; j < 64 ; j++)
        {
            if (i == j) continue;
            src.push_back(i);
            dst.push_back(j);
        }
    }

    for (int i = 0 ; i < 16 ; i++)
    {
        header.vertex_range[i] = UNUSED_ICON;
        header.edge_count[i] = 0;
        header.attr[i] = 0;
    }

    for (int i = 0 ; i < 16 ; i++)
    {
        header.vertex_range[i] = (i == 0) ? 1 : header.vertex_range[i - 1] + 1;
    }

    for (int i = 0 ; i < src.size() ; i+=16)
    {
        for (int j = i ; j < i + 16 && j < src.size() ; j++)
        {
            int s = src[j];
            int d = dst[j];
            insert_edge(s, d, data, header);
        }
        int enlarged_segs[VERTEX_PER_PMA];
        for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
        {
            enlarged_segs[v] = 0;
        }
        for (int v = 0 ; v < 16 ; v++)
        {
            int max_edge;
            int start_seg = (v == 0) ? 0 : header.vertex_range[v - 1];
            int end_seg = header.vertex_range[v];
            max_edge = (end_seg - start_seg) * SEGMENT_SIZE * DENSITY_BOUND;
            if (header.edge_count[v] > max_edge)
            {
                enlarged_segs[v] += (end_seg - start_seg);
            }
        }
        int total_extra_segs = 0;
        for (int v = 0 ; v < VERTEX_PER_PMA;v++)
        {
            total_extra_segs += enlarged_segs[v];
            header.vertex_range[v] += total_extra_segs;
        }
        rebalance(data, header);
        show_raw_pma(data, header);
    }
    return 0;
}