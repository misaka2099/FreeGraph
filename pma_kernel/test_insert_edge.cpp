#include <cstring>
#include <hls_stream.h>
#include <iostream>
#include <thread>
#include <future>
#include "data_type.h"
#include "test_insert_edge.h"

void set(int start, int len, BYTE* bitmap)
{
    for (int i = start; i < start + len; i++)
    {
        bitmap[i / 8] |= ((BYTE)1 << (i % 8));
    }
}

bool is_free(int start, int len, int &first_set , BYTE* bitmap)
{
    for (int i = start; i < start + len; i++)
    {
        if (bitmap[i / 8] & (1 << (i % 8)))
        {
            first_set = i;
            return false;
        }
    }
    return true;
} 

void allocate(int &start, int len, BYTE* bitmap)
{
    start = -1;
    if (len == 0) return;
    int first_set = -1;
    for (int i = 0 ; i < CHANNEL_CAPACITY * CHANNEL_NUM ; i++)
    {
        if (is_free(i, len, first_set, bitmap))
        {
            start = i;
            set(start, len, bitmap);
            return;
        }
        else
        {
            i = first_set;
        }
    }
}

void reset(int start, int len, BYTE* bitmap)
{
    for (int i = start; i < start + len; i++)
    {
        bitmap[i / 8] &= ~((BYTE)1 << (i % 8));
    }
}

void deallocate(int start, int len, BYTE* bitmap)
{
    if (len == 0) return;
    reset(start, len, bitmap);
}

BYTE storage[10000000];
BYTE bm[10000000];
VEC_INFO pos_addr[10000];
VEC_INFO neg_addr[10000];
int main()
{
    memset(storage, 0,  sizeof(storage));
    memset(bm,0,sizeof(bm));
    memset(pos_addr, 0 , sizeof(pos_addr));
    memset(neg_addr, 0 , sizeof(neg_addr));
    hls::stream<PMA_UPDATE_MESSAGE> cmd;
    hls::stream<COMMAND_STREAM> ack_back_to_router;
    hls::stream<VEC_INFO_STREAM> vec_info_to_mem_stream;
    
    hls::stream<COMMAND_STREAM> cmd_to_mem_stream;
    hls::stream<VEC_INFO_STREAM> vec_info_back_stream;
    
    VEC_INFO vec;
    vec.range(31,0) = 0;
    vec.range(63,32) = sizeof(int) * SEGMENT_SIZE * VERTEX_PER_PMA + sizeof(PMA_HEADER);
    neg_addr[0] = vec;
    // 1. 准备输入数据
    PMA_HEADER pma_header;
    for (int i = 0 ; i < 16 ; i++)
    {
        pma_header.attr[i] = 0;
        pma_header.edge_count[i] = 0;
        pma_header.vertex_range[i] = i+1;
    }
    pma_header.edge_count[0] = 48;
    memcpy(storage, &pma_header, sizeof(PMA_HEADER));
    int *pma_0_storage = (int*)(storage + sizeof(PMA_HEADER));
    for (int i = 0 ; i < 16 * SEGMENT_SIZE ; i++)
    {
        pma_0_storage[i] = UNUSED_ICON;
    }
    for (int i = 0 ; i < 48 ; i++)
    {
        pma_0_storage[i] = i;
    }
    int bin;
    allocate(bin, sizeof(PMA_HEADER) + 16 *SEGMENT_SIZE *sizeof(int), bm);
    for (int k = 0 ; k < 16 ; k++)
    {
        PMA_UPDATE_MESSAGE msg;
        msg.data.range(31,0) = 0;
        msg.data.range(63,32) = k+48;
        msg.data.range(98,98) = IN_DIRECTION;
        msg.last = k == 15 ? 1 : 0;
        cmd.write(msg);
    }
        
    
    // 发送 STOP 信号，并设置 last=1
    PMA_UPDATE_MESSAGE stop; 
    stop.data = PMA_STOP; 
    stop.last = 1;
    cmd.write(stop);
    std::cout << "STARTING KERNEL" <<std::endl;
   // 2. 异步启动内核，使其在后台线程运行
    // auto kernel_future = std::async(std::launch::async, [&]() {
    //         pma_insert_edge(
    //         pos_addr,
    //         neg_addr,
    //         storage,
    //         cmd,
    //         ack_back_to_router,
    //         //mem operation
    //         vec_info_to_mem_stream,
    //         cmd_to_mem_stream,
    //         vec_info_back_stream
    //     );
    // });
    std::thread kernel_thread([&]() {
        pma_insert_edge(
            pos_addr,
            neg_addr,
            storage,
            storage,
            storage,
            storage,
            storage,
            cmd,
            vec_info_to_mem_stream,
            cmd_to_mem_stream,
            vec_info_back_stream,
            ack_back_to_router
        );
    });
    int request = 0;
    while (request < 1)
    {
        std::cout << "Host: Waiting for memory request " << request << "..." << std::endl;
        COMMAND_STREAM cmd;
        // while (cmd_to_mem_stream.empty() && vec_info_to_mem_stream.empty()) {}
        if (!cmd_to_mem_stream.read_nb(cmd)) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            std::cout << "NOT REQUIERD" <<std::endl;
            continue;
        }
        if (cmd.data == 0)
        {
            std::cout << "NOT VALID REQUEST "<<std::endl;
            continue;
        }
        std::cout << "HOST RECEIVE COMMAND " << cmd.data << std::endl;
        auto to_mem_vec = vec_info_to_mem_stream.read();
        int len = to_mem_vec.data.range(63,32);
        std::cout << "LEN REQUEST " << len <<std::endl; 
        int start = -1;
        allocate(start, len, bm);
        VEC_INFO back;
        back.range(31,0) = start;
        back.range(63,32) = len;
        std::cout << "Host: Allocated " << len << " bytes at address " << start << ". Responding to kernel." << std::endl;
        vec_info_back_stream.write({.data=back});
        request++;
    }
    std::cout << "AFTER ALLOC" <<std::endl;
    // std::thread allocator_thread(simulate_memory_allocator, std::ref(cmd_to_mem_stream), std::ref(vec_info_to_mem_stream), std::ref(vec_info_back_stream), bm);
    
    kernel_thread.join();
    // allocator_thread.join();
    // 3. 主机模拟内存分配器，与内核并行交互
    // 4. 等待内核处理完所有请求并返回确认信息
    // 100个正常ack + 1个STOP ack

    // 5. 等待内核线程完全结束
    // kernel_future.get();
    ack_back_to_router.read();
    PMA_HEADER check_header;
    auto pma_idx_info = neg_addr[0];
    std::cout << "START = " << int(pma_idx_info.range(31,0)) << " LEN = " <<  int(pma_idx_info.range(63,32)) << std::endl;
    memcpy(&check_header, storage + int(pma_idx_info.range(31,0)),sizeof(check_header));
    for (int i = 0 ; i < 16 ; i++)
    {
        std::cout << "VERTEX_RANGE " << i << " = " << check_header.vertex_range[i] << std::endl;;
    }
    for (int i = 0 ; i < 16 ; i++)
    {
        std::cout << "EDGE COUNT " << i << " = " << check_header.edge_count[i] << std::endl;;
    }
    std::cout << std::endl;
    int check_storage[17*SEGMENT_SIZE];
    memcpy(check_storage, storage, check_header.vertex_range[15] * SEGMENT_SIZE * sizeof(int));
    for (int i = 0 ; i < 16 ; i++)
    {
        int start_seg = i == 0 ? 0 : check_header.vertex_range[i-1];
        int end_seg = check_header.vertex_range[i];
            std::cout << "SRC " << i << " : ";
        for (int j = start_seg * SEGMENT_SIZE ; j < end_seg * SEGMENT_SIZE ; j++)
        {
            if (check_storage[j] != UNUSED_ICON)
            std::cout << check_storage[j] << " ";
        }   
        std::cout << std::endl;
    }
    std::cout << "Simulation finished successfully." << std::endl;
    return 0;
}