#ifndef __KERNEL_MEMORY_H_
#define __KERNEL_MEMORY_H_
#include "stdint.h"
#include "bitmap.h"
struct virtual_addr{
    struct bitmap vaddr_bitmap;//虚拟地址用到的位图结构
    uint32_t vaddr_start;//虚拟地址起始地址
};
extern struct pool kernel_pool,user_pool;
void mem_init(void);
enum pool_flags{
    PF_KERNEL=1, //内核内存池
    PF_USER=2//用户内存池
};
void *get_kernel_pages(uint32_t pg_cnt);
#define PG_P_1   1 //页表项或目录项存在属性位
#define PG_P_0   0   //页表项或目录项存在属性位
#define PG_RW_R 0   // R/W 属性位 读/执行
#define PG_RW_W 2   // R/W 属性位 读/写/执行
#define PG_US_S 0   // U/S 属性位 ，系统级
#define PG_US_U 4  // U/S 属性位，用户级
#endif