#pragma once 
#include "data_type.h"
void binary_search(int val, int left, int right, int *storage, int &out_idx, int& out_val);
void insert_edge(int src, int dst, int* pma_data, PMA_HEADER& pma_header);
void rebalance(int* pma_data, PMA_HEADER& pma_header);