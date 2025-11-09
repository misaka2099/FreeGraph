#include <hls_stream.h>
#include <iostream>
#include <thread>
#include <future>
#include "data_type.h"
#include "test_insert_vertex.h"

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
    
    // 1. 准备输入数据
    for (int i = 0 ; i < 10 ; i++)
    {
        PMA_UPDATE_MESSAGE msg;
        msg.data.range(31,0) = i*16;
        msg.data.range(98,98) = IN_DIRECTION;
        msg.last = 0;
        cmd.write(msg);
    }
    
    // 发送 STOP 信号，并设置 last=1
    PMA_UPDATE_MESSAGE stop; 
    stop.data = PMA_STOP; 
    stop.last = 1;
    cmd.write(stop);

   // 2. 异步启动内核，使其在后台线程运行
    auto kernel_future = std::async(std::launch::async, [&]() {
        pma_insert_vertex(
            pos_addr,
            neg_addr,
            storage,
            cmd,
            ack_back_to_router,
            vec_info_to_mem_stream,
            cmd_to_mem_stream,
            vec_info_back_stream
        );
    });

    // 3. 主机模拟内存分配器，与内核并行交互
    int request = 0;
    while (request < 10)
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
    // 4. 等待内核处理完所有请求并返回确认信息
    // 100个正常ack + 1个STOP ack
    for (int i = 0 ; i < 10 ; i++)
    {
        auto ack = ack_back_to_router.read();
        std::cout << "Host: Received ack " << i << " from kernel." << std::endl;
    }
    
    // 5. 等待内核线程完全结束
    kernel_future.get();
    for (int i = 0 ; i < 10 ; i++)
    {
        VEC_INFO info = neg_addr[i];
        int start = info.range(31,0);
        int len = info.range(63,32);
        PMA_HEADER header;
        memcpy(&header, storage+start, sizeof(PMA_HEADER));
        for (int j = 0 ; j < 16 ; j++)
        {
            if (header.vertex_range[j] != j && header.edge_count[j] != 0)
            {
                std::cout << "ERROR AT HEADER PMA IDX "<< i << std::endl;
                return 1;
            }
        }
        int check_pma[SEGMENT_SIZE*16];
        memcpy(check_pma, storage+start+sizeof(PMA_HEADER), len - sizeof(PMA_HEADER));
        for (int j = 0 ; j < 16 * SEGMENT_SIZE ; j++)
        {
            if (check_pma[j] != UNUSED_ICON)
            {
                std::cout << "ERROR AT RAW PMA IDX "<< i << std::endl;
                return 1;
            }
        }
    }
    std::cout << "Simulation finished successfully." << std::endl;
    return 0;
}