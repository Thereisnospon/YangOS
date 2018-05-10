#include "memory.h"
#include "memory.h"
#include "stdint.h"
#include "global.h"
#include "debug.h"
#include "print.h"
#include "string.h"

#define PG_SIZE 4096

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

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
    struct bitmap pool_bitmap; //内存池用到的位图结构
    uint32_t phy_addr_start;   //内存池管理的物理内存的起始地址
    uint32_t pool_size;        //内存池字节容量
};
struct pool kernel_pool, user_pool; //内核内存池，用户内存池
struct virtual_addr kernel_vaddr;   //为内核分配虚拟地址

/*
pf表示的虚拟内存池申请pg_cnt个虚拟页，成功返回虚拟页的起始地址，失败返回 NULL
*/
static void *vaddr_get(enum pool_flags pf, uint32_t pg_cnt)
{
    int vaddr_start = 0, bit_idx_start = -1;
    uint32_t cnt = 0;
    if (pf == PF_KERNEL)
    {
        bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
        if (bit_idx_start == -1)
        {
            return NULL;
        }
        while (cnt < pg_cnt)
        {
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + (cnt++), 1);
        }
        vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
    }
    else
    {
        //用户内存池
    }
    return (void *)vaddr_start;
}
//得到虚拟地址 vaddr 对应的 pte 指针
uint32_t *pte_ptr(uint32_t vaddr)
{
    /*
      1023 页表项放的是页表地址。 1023=0x3ff, 0x3ff 移动到高10 位变为 0xffc00000
      第一次欺骗: 高10 位的 0xffc0000 索引到了 1023 的页目录项（里面放的是 页目录表的地址，而cpu把它当做 页表地址)

      把 vaddr 高10 位的 pde 索引拿出来，当做新地址的中间 10 位，让cpu把它当做 pte 索引
      vaddr & 0xffc00000 拿到 vaddr 的高10位， 再又移动 10 位。 就移动到了中间10位。
      第二次欺骗：让cpu把 vaddr 的 pde索引当做 pte 索引

      把vaddr 的中间10 位 （pte 索引） 当做 cpu 眼中的 12 位的页内偏移
      先获得 vaddr 中间10位 (pte索引），然后乘4(每个页目录项4字节)
      第三次欺骗: 让cpu把 vaddr 的 pte 索引*4 当做 页内偏移

      最后相加 高中低 得到 页表项的虚拟地址

    */
    uint32_t *pte = (uint32_t *)(0xffc00000 +
                                 ((vaddr & 0xffc00000) >> 10) +
                                 PTE_IDX(vaddr) * 4);
    return pte;
}
//得到虚拟地址vaddr 对应的 pde 指针
uint32_t *pde_ptr(uint32_t vaddr)
{
    /*
        0xfffff000 的高10位: 0x3ff ,中 10 位也是 0x3ff
        通过高10位，1023 拿到了 页目录表地址
        中10位，1023 去索引 1023 的页表项 （其实还是索引到了1023 的页目录项
        再加上 pde 索引
    */
    uint32_t *pde = (uint32_t *)((0xfffff000) + PDE_IDX(vaddr) * 4);
    return pde;
}

/*
    在 m_pool 指向的物理内存中分配1个物理页,成功返回页框的物理地址
*/
static void *palloc(struct pool *m_pool)
{
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
    if (bit_idx == -1)
    {
        return NULL;
    }
    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
    uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
    return (void *)page_phyaddr;
}
#define PAGE_EXIST_MASK 0x00000001

/*
页表中添加虚拟地址 _vaddr 与物理地址 _page_phyaddr 的映射
*/
static void page_table_add(void *_vaddr, void *_page_phyaddr)
{
    uint32_t vaddr = (uint32_t)_vaddr;
    uint32_t page_phyaddr = (uint32_t)_page_phyaddr;
    uint32_t *pde = pde_ptr(vaddr);
    uint32_t *pte = pte_ptr(vaddr);

    /************************   注意   *************************
    * 执行*pte,会访问到空的pde。所以确保pde创建完成后才能执行*pte,
    * 否则会引发page_fault。因此在*pde为0时,*pte只能出现在下面else语句块中的*pde后面。
    * *********************************************************/
    /* 先在页目录内判断目录项的P位，若为1,则表示该表已存在 */
    if (*pde & PAGE_EXIST_MASK)
    {
        ASSERT(!(*pte & PAGE_EXIST_MASK));
        if (!(*pte & PAGE_EXIST_MASK))
        {
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        }
        else
        {
            //目前不会执行到这里，因为上面的 ASSERT 会先执行
            PANIC("pte repeat");
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        }
    }
    else
    {
        //页目录项不存在，先创建页目录项再创建页表项
        //页表中的页框一律从内核空间分配
        uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);

        /* 分配到的物理页地址pde_phyaddr对应的物理内存清0,
        * 避免里面的陈旧数据变成了页表项,从而让页表混乱.
        * pte 是 vaddr 所在的页表项的虚拟地址。（高20位定位到了页表地址，低12位为0，偏移地址为0，得到了新申请页的虚拟地址）
        * 访问到pde对应的物理地址,用pte取高20位便可.
        * 因为pte是基于该pde对应的物理地址内再寻址,
        * 把低12位置0便是该pde对应的物理页的起始*/
        memset((void *)((int)pte & 0xfffff000), 0, PG_SIZE); //把新申请的物理页清0

        ASSERT(!(*pte & 0x00000001));
        *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1); // US=1,RW=1,P=1
    }
}

void *malloc_page(enum pool_flags pf, uint32_t pg_cnt)
{
    ASSERT(pg_cnt > 0 && pg_cnt < 3840);
    /***********   malloc_page的原理是三个动作的合成:  ***********
      1通过vaddr_get在虚拟内存池中申请虚拟地址
      2通过palloc在物理内存池中申请物理页
      3通过page_table_add将以上得到的虚拟地址和物理地址在页表中完成映射
    ***************************************************************/
    void *vaddr_start = vaddr_get(pf, pg_cnt);
    if (vaddr_start == NULL)
    {
        return NULL;
    }
    uint32_t vaddr = (uint32_t)vaddr_start;
    uint32_t cnt = pg_cnt;
    struct pool *mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;

    //因为虚拟地址连续，但是物理地址不一定连续，所以逐个映射
    while (cnt-- > 0)
    {
        void *page_phyaddr = palloc(mem_pool);
        if (page_phyaddr == NULL) //失败时应该把申请的虚拟地址和物理页全部回滚
        {                         //将来完成内存回收时再补充
            return NULL;
        }
        page_table_add((void *)vaddr, page_phyaddr); //页表中映射
        vaddr += PG_SIZE;                            //下一个虚拟页
    }
    return vaddr_start;
}

//内核物理内存池申请 pg_cnt 页内存
void * get_kernel_pages(uint32_t pg_cnt){
    void * vaddr=malloc_page(PF_KERNEL,pg_cnt);
    if(vaddr!=NULL){
        memset(vaddr,0,pg_cnt*PG_SIZE);
    }
    return vaddr;
}

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
void mem_init()
{
    put_str("mem_init start\n");
    uint32_t mem_bytes_total = (*(uint32_t *)(0xb00));
    mem_pool_init(mem_bytes_total);
    put_str("meme_init done\n");
}