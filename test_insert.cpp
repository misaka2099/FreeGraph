#include <cstring>
#include <iostream>
#include "data_type.h"
#include "ap_int.h"
#include "hls_print.h"
BYTE storage[100000];
PMA_HEADER public_header;

void print_pma(
    
)
{
    VEC_INFO vec_info = 0;
    int start = vec_info.range(31,0);
    int len =  vec_info.range(63,32);
    auto pma_idx = 0;
    std::cout << "PMA " << pma_idx << " ADDR START FROM " << start << " LEN " << len << std::endl; 
    PMA_HEADER header;
    memcpy(&header, storage + start, sizeof(PMA_HEADER));
    int *pma_data = (int*)(storage + sizeof(PMA_HEADER) + start);
    for (int i = 0 ; i < 16 ; i++)
    {
        int src = pma_idx * 16  + i;
        int edge_count = header.edge_count[i];
        int seg_start = i == 0 ? 0 : header.vertex_range[i-1];
        int seg_end = header.vertex_range[i];
        std::cout << "vertex: " << src << " has " << edge_count << " neighbors" << std::endl;
        for (int j = seg_start ; j < seg_end ; j ++)
        {
            for (int k = 0 ; k < SEGMENT_SIZE ; k++)
            {
                std::cout <<pma_data[j*SEGMENT_SIZE + k] << " ";
            }
        }
        std::cout << std::endl;
    }
}
void init_pma_space(

)
{
    for (int i = 0 ; i < 16 ; i++)
    {
        public_header.vertex_range[i] = i + 1;
        public_header.edge_count[i] = 0;
    }
    memcpy(storage, &public_header, sizeof(PMA_HEADER));
    int segment[SEGMENT_SIZE];
    for (int i = 0 ; i < SEGMENT_SIZE ; i++) segment[i] = -1;
    for (int i = 0 ; i < 16 ; i++)
    {
        int addr = sizeof(PMA_HEADER) + i * SEGMENT_SIZE * sizeof(int);
        memcpy(storage + addr, segment, SEGMENT_SIZE * sizeof(int));
    }
}
void read_seg(
    int seg,
    int* buffer
)
{
    memcpy(buffer, storage + sizeof(PMA_HEADER) + seg * SEGMENT_SIZE * sizeof(int), SEGMENT_SIZE * sizeof(int));
}
void write_seg(
    int seg,
    int* buffer
)
{
    memcpy(storage + sizeof(PMA_HEADER) + seg * SEGMENT_SIZE * sizeof(int), buffer, SEGMENT_SIZE * sizeof(int));
}
void get_bs_helper(
    int src,
    int*bs_helper,
    int &cache_size
)
{
    #pragma HLS INLINE
    int local_src = src % VERTEX_PER_PMA;
    int low_seg = local_src == 0 ? 0 : public_header.vertex_range[local_src - 1];
    int high_seg = public_header.vertex_range[local_src];
    cache_size = high_seg - low_seg;
    int segment_buffer[SEGMENT_SIZE];
    hls::print("Enter get_bs_helper\n");
    hls::print("low_seg: %d\n", low_seg);
    hls::print("high_seg: %d\n", high_seg);
    hls::print("cache_size: %d\n", cache_size);
    for (int i = 0 ; i < cache_size ; i++)
    {
        #pragma HLS PIPELINE off
        read_seg(
            low_seg + i,
            segment_buffer
        );
        hls::print("Caching Segment %d\n", i+low_seg);
        bs_helper[i] = segment_buffer[0];
    }
}
void sub_search(
    int* cache_bs_helper,
    int cache_size, //num of int
    int dst,
    int &searched_seg_number
)
{
    #pragma HLS INLINE
    int low = 0;
    int high = cache_size - 1;
    searched_seg_number = -1;
    hls::print("Enter sub_search\n");
    hls::print("cache_size: %d\n", cache_size);
    hls::print("dst: %d\n", dst);
    while (low <= high) {
        #pragma HLS PIPELINE off
        int mid = (low + high) / 2;
        if (cache_bs_helper[mid] == dst) {
            searched_seg_number = mid;
            return;
        } else if (cache_bs_helper[mid] < dst) {
            low = mid + 1;
            searched_seg_number = mid; // Record the last position where cache_bs_helper[mid] < src
        } else {
            high = mid - 1;
        }
    }
}

struct inserted_seg
{
    int old_vertex_relative_seg_number[16];
    int vertex_number[16];
};
void search_and_insert_cu(

)
{
    printf("BINARY_SEARCH_AND_INSERT\n");
        inserted_seg inserted_seg_info;
        for (int i = 0 ; i < 16 ; i++)
        {
            inserted_seg_info.old_vertex_relative_seg_number[i] = -1;
            inserted_seg_info.vertex_number[i] = -1;
        }
        auto header = public_header;
        int count = 16;
        int find_count = 0;

        // Cache mechanism
        int current_cached_id = -1;
        bool is_dirty = false;
        int last_src = -1;
        int last_found_seg = -1;

        int storage[SEGMENT_SIZE];
        #pragma HLS ARRAY_PARTITION variable=storage cyclic factor=64

        for (int i = 0 ; i < SEGMENT_SIZE ; i++) storage[i] = -1;

        int bs_helper[1024];
        #pragma HLS BIND_STORAGE variable=bs_helper type=RAM_2P impl=URAM
        int bs_helper_size = 0;
        ap_uint<64> insert_ops[16];
        int insert_ops_count = 0;
        ap_uint<64> delete_edge_ops[16];
        int delete_edge_ops_count = 0;
        ap_uint<64> delete_vertex_ops[16];
        int delete_vertex_ops_count = 0;
        ap_uint<128> edge_info_stream[16]=
        {
            (ap_uint<128>(PMA_INSERT_EDGE) << 32) | (ap_uint<128>(0) << 96) | (ap_uint<128>(1) << 64),
            (ap_uint<128>(PMA_INSERT_EDGE) << 32) | (ap_uint<128>(0) << 96) | (ap_uint<128>(2) << 64),
            (ap_uint<128>(PMA_INSERT_EDGE) << 32) | (ap_uint<128>(0) << 96) | (ap_uint<128>(3) << 64),
            (ap_uint<128>(PMA_INSERT_EDGE) << 32) | (ap_uint<128>(0) << 96) | (ap_uint<128>(4) << 64),
            (ap_uint<128>(PMA_INSERT_EDGE) << 32) | (ap_uint<128>(0) << 96) | (ap_uint<128>(5) << 64),
            (ap_uint<128>(PMA_INSERT_EDGE) << 32) | (ap_uint<128>(0) << 96) | (ap_uint<128>(6) << 64),
            (ap_uint<128>(PMA_INSERT_EDGE) << 32) | (ap_uint<128>(0) << 96) | (ap_uint<128>(7) << 64),
            (ap_uint<128>(PMA_INSERT_EDGE) << 32) | (ap_uint<128>(0) << 96) | (ap_uint<128>(8) << 64),
            (ap_uint<128>(PMA_INSERT_EDGE) << 32) | (ap_uint<128>(0) << 96) | (ap_uint<128>(9) << 64),
            (ap_uint<128>(PMA_INSERT_EDGE) << 32) | (ap_uint<128>(0) << 96) | (ap_uint<128>(10) << 64),
            (ap_uint<128>(PMA_INSERT_EDGE) << 32) | (ap_uint<128>(0) << 96) | (ap_uint<128>(11) << 64),
            (ap_uint<128>(PMA_INSERT_EDGE) << 32) | (ap_uint<128>(0) << 96) | (ap_uint<128>(12) << 64),
            (ap_uint<128>(PMA_INSERT_EDGE) << 32) | (ap_uint<128>(0) << 96) | (ap_uint<128>(13) << 64),
            (ap_uint<128>(PMA_INSERT_EDGE) << 32) | (ap_uint<128>(0) << 96) | (ap_uint<128>(14) << 64),
            (ap_uint<128>(PMA_INSERT_EDGE) << 32) | (ap_uint<128>(0) << 96) | (ap_uint<128>(15) << 64)
        };
        for (int i = 0 ; i < count ; i++)
        {
            auto edge_info = edge_info_stream[i];
            switch (edge_info.range(63,32).to_uint()) 
            {
                case PMA_INSERT_EDGE:
                    insert_ops[insert_ops_count++] = edge_info.range(127,64);
                    break;
                case PMA_DELETE_EDGE:
                    delete_edge_ops[delete_edge_ops_count++] = edge_info.range(127,64);
                    break;
                case PMA_DELETE_VERTEX:
                    delete_vertex_ops[delete_vertex_ops_count++] = edge_info.range(127,64);
                    break;
                default:
                    // Handle unknown opcode if necessary
                    break;
            }
        }
        hls::print("Insert Ops Count: %d\n", insert_ops_count);
        hls::print("Delete Edge Ops Count: %d\n", delete_edge_ops_count);
        hls::print("Delete Vertex Ops Count: %d\n", delete_vertex_ops_count);
        for (int i = 0 ; i < delete_vertex_ops_count; i++)
        {
            #pragma HLS PIPELINE off
            int src = delete_vertex_ops[i].range(63,32);
            int dst = delete_vertex_ops[i].range(31,0);

            int local_src = src % VERTEX_PER_PMA;
            int low_seg = local_src == 0 ? 0 : header.vertex_range[local_src - 1];
            int high_seg = header.vertex_range[local_src];              
            // Prepare empty buffer
            for(int k=0; k<SEGMENT_SIZE; k++) storage[k] = UNUSED_ICON;
            // Iterate through segments
            for (int s = low_seg; s < high_seg; s++)
            {
                // Write to mem
                write_seg(s, storage);
            }
            
            // Update header state
            header.edge_count[local_src] = 0;
        }
        int last_bs_cached_src = -1;
        for (int i = 0 ; i < insert_ops_count; i++)
        {
            #pragma HLS PIPELINE off
            int src = insert_ops[i].range(63,32);
            int dst = insert_ops[i].range(31,0);

            int local_src = src % VERTEX_PER_PMA;
            if (last_bs_cached_src != src)
            {
                get_bs_helper(src, bs_helper, bs_helper_size);
                last_bs_cached_src = src;
            }
            int searched_seg_number = -1;
            sub_search(bs_helper, bs_helper_size, dst, searched_seg_number);
            if (searched_seg_number == -1) searched_seg_number = 0;//insert at the first segment and the first element.
            inserted_seg_info.old_vertex_relative_seg_number[i] = searched_seg_number;
            inserted_seg_info.vertex_number[i] = local_src;
        }
        hls::print("Finish searching for insert positions\n");
        ap_uint<32> delete_segs[16];
        for (int i = 0 ; i < delete_edge_ops_count; i++)
        {
            #pragma HLS PIPELINE off
            int src = delete_edge_ops[i].range(63,32);
            int dst = delete_edge_ops[i].range(31,0);

            int local_src = src % VERTEX_PER_PMA;
            if (last_bs_cached_src != src)
            {
                get_bs_helper( src, bs_helper, bs_helper_size);
                last_bs_cached_src = src;
            }
            int searched_seg_number = -1;
            sub_search(bs_helper, bs_helper_size, dst, searched_seg_number);
            delete_segs[i] = searched_seg_number;
        }
        hls::print("Finish searching for delete positions\n");
        for (int i = 0 ; i < delete_edge_ops_count ; i++)
        {
            #pragma HLS PIPELINE off
            int src = delete_edge_ops[i].range(63,32);
            int dst = delete_edge_ops[i].range(31,0);

            int local_src = src % VERTEX_PER_PMA;
            int low_seg = local_src == 0 ? 0 : header.vertex_range[local_src - 1];
            int target_seg = delete_segs[i] + low_seg;
            if (current_cached_id != target_seg) {
                if (is_dirty && current_cached_id != -1) {
                    write_seg(current_cached_id, storage);
                }
                read_seg(target_seg, storage);
                current_cached_id = target_seg;
            }
            int found_idx_for_del = -1;
            for (int j = 0; j < SEGMENT_SIZE; j++) {
                #pragma HLS LOOP_TRIPCOUNT min=0 max=64
                int storage_val = storage[j];
                if (storage_val == dst) {
                    found_idx_for_del = j;
                    break;
                }
            }
            if (found_idx_for_del == -1) continue; // Edge not found, skip deletion
            for (int j = found_idx_for_del + 1; j < SEGMENT_SIZE; j++)
            {
                storage[j-1] = storage[j];
            }
            storage[SEGMENT_SIZE - 1] = UNUSED_ICON;
            is_dirty = true;
            header.edge_count[local_src]--;
        }
        hls::print("Finish deleting edges\n");
        for (int i = 0 ; i < insert_ops_count ; i++)
        {
            #pragma HLS PIPELINE off
            int src = insert_ops[i].range(63,32);
            int dst = insert_ops[i].range(31,0);

            int local_src = src % VERTEX_PER_PMA;
            int low_seg = local_src == 0 ? 0 : header.vertex_range[local_src - 1];
            int target_seg = inserted_seg_info.old_vertex_relative_seg_number[i] + low_seg;
            if (current_cached_id != target_seg) {
                if (is_dirty && current_cached_id != -1) {
                    write_seg(current_cached_id, storage);
                }
                read_seg(target_seg, storage);
                current_cached_id = target_seg;
            }
            int found_idx_for_insert = -1;
            for (int j = 0; j < SEGMENT_SIZE; j++) {
                #pragma HLS LOOP_TRIPCOUNT min=0 max=64
                int storage_val = storage[j];
                if (storage_val >= dst) {
                    found_idx_for_insert = j;
                    break;
                }
                if (storage_val == UNUSED_ICON) {
                    found_idx_for_insert = j;
                    break;
                }
            }
            for (int j = SEGMENT_SIZE-1; j > found_idx_for_insert; j--)
                storage[j] = storage[j-1];
            storage[found_idx_for_insert] = dst;
            is_dirty = true;
            header.edge_count[local_src]++;
        }
        hls::print("Finish inserting edges\n");
        
        if (is_dirty && current_cached_id != -1) {
            write_seg(current_cached_id, storage);
            is_dirty = false;
        }
}
int main()
{
    init_pma_space();
    print_pma();
    search_and_insert_cu();
    print_pma();
}