#include "memory.h"
#include "stdint.h"
#include "print.h"
#define PG_SIZE 4096
/***************  位图地址 ********************
 * 因为0xc009f000是内核主线程栈顶，0xc009e000是内核主线程的pcb.
 * 一个页框大小的位图可表示128M内存, 位图位置安排在地址0xc009a000,
 * 这样本系统最大支持4个页框的位图,即512M */
#define MEM_BITMAP_BASE 0xc009a000
/*************************************/

// ; <--+-------------+--> 0x9f000 esp 内核栈 栈底
// ; <-------------------> 0x9efff
// ;    |             |
// ;    |    PCB      |  内核 PCB (0x9e000~0x9efff) 4KB 一个自然页
// ;    |             |
// ; <--+-------------+--> 0x9E000
// ; <------------------->
// ;    |             |
// ;    |    位图      |   共4页
// ;    |             |
// ; <--+-------------+--> 0x9A000

// 0xc0000000 是内核从虚拟地址 3g 起
// 0x100000 是越过低端 1MB 内存，使得虚拟地址在逻辑上连续
#define K_HEAP_START 0xc0100000

//内存池结构
struct pool
{
    struct bitmap pool_bitmap;        //内存池用到的位图结构
    uint32_t phy_addr_start;          //内存池管理的物理内存的起始地址
    uint32_t pool_size;               //内存池字节容量
} ;
struct pool kernel_pool, user_pool; //内核内存池，用户内存池
struct virtual_addr kernel_vaddr;     //为内核分配虚拟地址

static void mem_pool_init(uint32_t all_mem)
{
    put_str("mem_pool_init start\n");
    //page_table_size 记录目录表和页表占用的字节大小。= 页目录表大小+页表大小
    //页目录 1 页框，0~768 页目录项指向同一个页表，共享1页框空间
    //769~1022 页目录项共指向254个页表，页表总大小 256*PG_SIZE
    uint32_t page_table_size = PG_SIZE * 256;
    //已使用内存字节数。低端1mb空间+页表大小page_table_size
    uint32_t used_mem = page_table_size + 0x100000;
    //目前可用内存字节数
    uint32_t free_mem = all_mem - used_mem;
    //可用的物理页数
    uint16_t all_free_pages = free_mem / PG_SIZE;

    uint16_t kernel_free_pages = all_free_pages / 2;
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;

    //为了简化位图操作，余数不做处理，坏处是这样会浪费内存
    //好处是不用做内存越界检查，因为位图表示的内存少于实际物理内存
    uint32_t kbm_length = kernel_free_pages / 8; //位图中1位表示1页
    uint32_t ubm_length = user_free_pages / 8;

    uint32_t kp_start = used_mem;                               //内核内存池起始地址
    uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE; //用户内存池起始地址

    kernel_pool.phy_addr_start = kp_start;
    user_pool.phy_addr_start = up_start;

    kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
    user_pool.pool_size = user_free_pages * PG_SIZE;

    kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
    user_pool.pool_bitmap.btmp_bytes_len = ubm_length;

    /*********    内核内存池和用户内存池位图   ***********
    *   位图是全局的数据，长度不固定。
    *   全局或静态的数组需要在编译时知道其长度，
    *   而我们需要根据总内存大小算出需要多少字节。
    *   所以改为指定一块内存来生成位图.
    *   ************************************************/
    // 内核使用的最高地址是0xc009f000,这是主线程的栈地址.(内核的大小预计为70K左右)
    // 32M内存占用的位图是2k.内核内存池的位图先定在MEM_BITMAP_BASE(0xc009a000)处.
    kernel_pool.pool_bitmap.bits = (void *)MEM_BITMAP_BASE;

    /* 用户内存池的位图紧跟在内核内存池位图之后 */
    user_pool.pool_bitmap.bits = (void *)(MEM_BITMAP_BASE + kbm_length);

    /******************** 输出内存池信息 **********************/
    put_str("      kernel_pool_bitmap_start:");
    put_int((int)kernel_pool.pool_bitmap.bits);
    put_str(" kernel_pool_phy_addr_start:");
    put_int(kernel_pool.phy_addr_start);
    put_str("\n");
    put_str("      user_pool_bitmap_start:");
    put_int((int)user_pool.pool_bitmap.bits);
    put_str(" user_pool_phy_addr_start:");
    put_int(user_pool.phy_addr_start);
    put_str("\n");

    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);

    //初始化内核虚拟地址位图
    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;

    //位图的数组指向一块没使用的内存，目前在内核内存池和用户内存池之外
    kernel_vaddr.vaddr_bitmap.bits = (void *)(MEM_BITMAP_BASE + kbm_length + ubm_length);

    kernel_vaddr.vaddr_start = K_HEAP_START;
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    put_str("mem_pool_init done\n");
}
void mem_init(){
    put_str("mem_init start\n");
    uint32_t mem_bytes_total=(*(uint32_t*)(0xb00));
    mem_pool_init(mem_bytes_total);
    put_str("meme_init done\n");
}