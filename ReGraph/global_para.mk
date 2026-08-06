#Little kernel setup
LITTLE_KERNEL_NUM=1
# dst 缓冲大小 = 可处理的最大顶点数。
# 上限分析: gather URAM = 32B × 顶点数 (8 PE × 4B/顶点), 131072 → 4MB ≈ SLR URAM 的 ~90%;
# dst 编码 20 位 (bit19 = dummy 标记) → 硬上限 524288。超过需多分区或改内核编码。
LITTLE_KERNEL_DST_BUFFER_SIZE=131072
#################################################################################################################
#Big kernel setup 
BIG_KERNEL_NUM=0
BIG_KERNEL_DST_BUFFER_SIZE=524288 #$(LITTLE_KERNEL_DST_BUFFER_SIZE)#
#################################################################################################################
#the size of destination buffer; patition size should be aligned with 8K size to improve the utilization of URAMs.
PARTITION_SIZE=$(LITTLE_KERNEL_DST_BUFFER_SIZE)
#the size of one ping or pong buffer
SRC_BUFFER_SIZE=4096 # 4096 vertices 16*2 KB per source vertex buffer for a scatter PE// 64 BRAMs per scatter PE
LOG2_SRC_BUFFER_SIZE=12 #4096 vertices 16*2 KB per source vertex buffer for a scatter PE// 64 BRAMs per scatter PE
#SRC_BUFFER_SIZE=16384*2 # 16384 vertices 64 KB per source vertex buffer for a scatter PE
HAVE_APPLY=1
#Wether the application needs apply phase on the accelerator side.
VERTEX_REORDER_ENABLE=1
#Wether we enable the vertex reordering technique for graph preporcessing.
#################################################################################################################