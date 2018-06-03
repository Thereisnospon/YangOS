#include "process.h"
#include "global.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"
#include "list.h"
#include "tss.h"
#include "interrupt.h"
#include "string.h"
#include "console.h"
#include "debug.h"
extern void intr_exit(void);

//构建用户进程初始上下文信息
void start_process(void *filename_)
{
    
    void *function = filename_;
    struct task_struct *cur = running_thread();
    cur->self_kstack += sizeof(struct thread_stack);
    //在 pcb 的 intr_stack 中准备好相关数据
    struct intr_stack *proc_stack = (struct intr_stack *)cur->self_kstack;
    //一些寄存器的初始化为0
    proc_stack->edi = proc_stack->esi = proc_stack->ebp = proc_stack->esp_dummy = 0;
    proc_stack->ebx = proc_stack->edx = proc_stack->ecx = proc_stack->eax = 0;
    //用户态用不上，直接初始化为0
    proc_stack->gs = 0; 
    proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
     // 待执行的用户程序地址 （模拟中断 cpu 压入的 eip 地址
    proc_stack->eip = function; 
    // 用户进程的代码段选择子 (特权级3  （模拟中断 cpu 压入 ，实现特权级降级的关键)
    proc_stack->cs = SELECTOR_U_CODE;
    // 退出中断进入任务后，必须保持继续响应中断 EFLAGS_IF 为 1
    // 不允许用户进程直接访问硬件 EFLAGS_IOPL 为 0
    proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    // 用户的 3 特权级的栈 （用户空间申请，（模拟 中断 cpu 压入
    proc_stack->esp = (void *)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PG_SIZE);
    // 用户的 3 特权级的栈段选择子， 模拟中断 cpu 压入
    proc_stack->ss = SELECTOR_U_DATA;
    //详情见kerneal.asm 的解释，说明中断时的操作
    asm volatile("movl %0,%%esp; jmp intr_exit" ::"g"(proc_stack)
                 : "memory");
}

//激活页表
void page_dir_activate(struct task_struct *p_thread)
{
    /*
     执行此函数时，当前任务可能是 内核线程 (当前线程只用在内核中)
     之所以要对内核线程也要重新安装页表，是因为上一次调度的可能是进程
     否则不恢复的话，内核线程会继续使用进程的页表
    */

    //如果是内核线程，重新填充页表为 0x100000 (内核使用的页表的物理地址)
    uint32_t pagedir_phy_addr = 0x100000;
    if (p_thread->pgdir != NULL) //是否为进程， pgdir 指向页目录表 地址 （页目录表 本身也要使用到 内存
    {
        pagedir_phy_addr = addr_v2p((uint32_t)p_thread->pgdir);
    }
    asm volatile("movl %0,%%cr3" ::"r"(pagedir_phy_addr)
                 : "memory");
}
//激活内核线程或进程的页表，更新tss中的esp为进程的0特权级栈
void process_activate(struct task_struct *p_thread)
{
    ASSERT(p_thread != NULL);

    //激活该进程或线程的页表
     page_dir_activate(p_thread);
    
    //更新进程的0特权级栈
    if (p_thread->pgdir)
    {
        update_tss_esp(p_thread);
    }
}
//创建页目录表，将当前页表表示的内核空间的pde复制，成功则返回页目录的虚拟地址，
uint32_t *create_page_dir(void)
{
    //用户进程的页表不能被用户直接访问，所以在内核空间申请
    uint32_t *page_dir_vaddr = get_kernel_pages(1);
    if (page_dir_vaddr == NULL)
    {
        console_put_str("create pge dir : get_kernel_page failed");
        return NULL;
    }
    /*
     先复制页目录， page_dir_vaddr + 0x300*4 是页目录的 第 768 项
     0xfffff000 = 1111_1111_11 11_1111_1111 0000_0000_0000 
     根据虚拟地址定位 前 10 位 全 1 ，定位到了 页目录表 的 最后一个页目录项
     由于在最后一个页目录项 放的是 页目录表的地址， 所以 cpu 把页目录表 当做了 页表， 然后去访问最后一个页表项（其实是页目录项）
     然后，继续访问到了 页目录表的地址，然后根据低 12 位， 定位到了页目录表的地址（也是第一个页目录项地址)
     0xfffff000 + 0x300*4 便指向了内核空间的页目录项起始
    */
    memcpy((uint32_t *)((uint32_t)page_dir_vaddr + 0x300 * 4), (uint32_t *)(0xfffff000 + 0x300 * 4), 1024);

    uint32_t new_page_dir_phy_addr = addr_v2p((uint32_t)page_dir_vaddr);
    //页目录地址放在最后一个页目录项，更新页目录地址为新页目录的物理地址
    page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1;
    return page_dir_vaddr;
}

void create_user_vaddr_bitmap(struct task_struct *user_prog)
{
    //用户程序自己的虚拟内存分配池， 虚拟内存分配池用到的位图 同样需要用到内存空间
    user_prog->userprog_vaddr.vaddr_start = USER_VADDR_START;
    //位图需要的内存页框数
    uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8, PG_SIZE);
    user_prog->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt);
    user_prog->userprog_vaddr.vaddr_bitmap.btmp_bytes_len = (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8;
    bitmap_init(&user_prog->userprog_vaddr.vaddr_bitmap);
}

void process_execute(void *filename, char *name)
{
    //内核空间申请 一页内存 作为 进程的 pcb
    struct task_struct *thread = get_kernel_pages(1);
    init_thread(thread, name, default_prio);
     //为用户进程创建管理虚拟地址空间的位图
    create_user_vaddr_bitmap(thread);
    thread_create(thread, start_process, filename);
    //创建页表，拷贝内核部分的页目录项
    thread->pgdir = create_page_dir();
    block_desc_init(thread->u_block_desc);
    enum intr_status old_status = intr_disable();
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);

    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);
    intr_set_status(old_status);
}
/*
*********************************************************************
**                      实现用户进程                                 **
*********************************************************************
                        创建过程
---------------------------------------------------------------------
process_execute(user_prog,....) {
    //内核空间申请 一页内存 作为 进程的 pcb
    thread = get_kernel_pages
    //初始 thread
    init_thread(thread,...)
    //为用户进程创建管理虚拟地址空间的位图
    create_user_vaddr_bitmap(thread)
    //创建线程，将 start_process 作为 kernel_thread 的 function 参数
    thread_create(thread,start_process,user_prog)
    //创建页表，拷贝内核部分的页目录项
    thread->pgdir=create_page_dir()
    list_append(&thread_ready_list,thread...)
    list_append(&thread_all_list,thread);
}
---------------------------------------------------------------------
                        执行过程
---------------------------------------------------------------------
时钟中断发生时，调度器 schedule 从就绪队列 thread_read_list 获取下一个任务
schedule() {
    //激活页表，更换 tss 中的 esp0
    process_activate(thread) {
        //激活页表，更换页表寄存器
        page_dir_activate(thread);
        //对于进程，更新 tss 中的 esp0 为该进程的 0 特权级栈 (pcb+PG_SIZE)pcb顶端
        if(thread->pgdir) {
            update_tss_esp(thread);
        }
    }
    switch_to()↓
}              ↓
               ↓
               ↓
               ↓ 调用
    kernel_thread(start_process,user_prog){
        调用 start_process ↓
    }                     ↓
                          ↓
        start_process() {
            
            //在 pcb 的 intr_stack 中准备好相关数据

            //一些寄存器的初始化为0
            proc_stack->edi = proc_stack->esi = proc_stack->ebp = proc_stack->esp_dummy = 0;
            proc_stack->ebx = proc_stack->edx = proc_stack->ecx = proc_stack->eax = 0;
            //用户态用不上，直接初始化为0
            proc_stack->gs = 0; 
            
            // 用户进程的数据段选择子（3特权级
            proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
            // 待执行的用户程序地址 （模拟中断 cpu 压入的 eip 地址
            proc_stack->eip = function; 
            // 用户进程的代码段选择子 (特权级3  （模拟中断 cpu 压入 ，实现特权级降级的关键)
            proc_stack->cs = SELECTOR_U_CODE;
            // 退出中断进入任务后，必须保持继续响应中断 EFLAGS_IF 为 1
            // 不允许用户进程直接访问硬件 EFLAGS_IOPL 为 0
            proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
            // 用户的 3 特权级的栈 （用户空间申请，（模拟 中断 cpu 压入
            proc_stack->esp = (void *)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PG_SIZE);
            // 用户的 3 特权级的栈段选择子， 模拟中断 cpu 压入
            proc_stack->ss = SELECTOR_U_DATA;
            //详情见kerneal.asm 的解释，说明中断时的操作
            asm volatile("movl %0,%%esp; jmp intr_exit" ::"g"(proc_stack)
                 : "memory");↓
        }                    ↓
                             ↓
            intr_exit{
                ↓
            }   ↓
                ↓
                user_prog()
*/
