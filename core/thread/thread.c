#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "debug.h"
#include "interrupt.h"
#include "print.h"
#include "memory.h"
#include "process.h"
#include "sync.h"
#include "stdio.h"
#include "file.h"
#include "fs.h"

#define PG_SIZE 4096

struct task_struct *main_thread;     //主线程pcb
struct list thread_ready_list;       //就绪队列
struct list thread_all_list;         //所有任务队列
static struct list_elem *thread_tag; //用于保存队列中的线程节点
struct lock pid_lock;
extern void switch_to(struct task_struct *cur, struct task_struct *next);
static void idle(void *arg) ;
struct task_struct *idle_thread; //idle线程
extern void init(void);

/* pid的位图,最大支持1024个pid */
uint8_t pid_bitmap_bits[128] = {0};

/* pid池 */
struct pid_pool
{
    struct bitmap pid_bitmap; // pid位图
    uint32_t pid_start;       // 起始pid
    struct lock pid_lock;     // 分配pid锁
} pid_pool;

/* 初始化pid池 */
static void pid_pool_init(void)
{
    pid_pool.pid_start = 1;
    pid_pool.pid_bitmap.bits = pid_bitmap_bits;
    pid_pool.pid_bitmap.btmp_bytes_len = 128;
    bitmap_init(&pid_pool.pid_bitmap);
    lock_init(&pid_pool.pid_lock);
}

/* 分配pid */
static pid_t allocate_pid(void)
{
    lock_acquire(&pid_pool.pid_lock);
    int32_t bit_idx = bitmap_scan(&pid_pool.pid_bitmap, 1);
    bitmap_set(&pid_pool.pid_bitmap, bit_idx, 1);
    lock_release(&pid_pool.pid_lock);
    return (bit_idx + pid_pool.pid_start);
}

/* 释放pid */
void release_pid(pid_t pid)
{
    lock_acquire(&pid_pool.pid_lock);
    int32_t bit_idx = pid - pid_pool.pid_start;
    bitmap_set(&pid_pool.pid_bitmap, bit_idx, 0);
    lock_release(&pid_pool.pid_lock);
}

/* fork进程时为其分配pid,因为allocate_pid已经是静态的,别的文件无法调用.
不想改变函数定义了,故定义fork_pid函数来封装一下。*/
pid_t fork_pid(void)
{
    return allocate_pid();
}

/* 回收thread_over的pcb和页表,并将其从调度队列中去除 */
void thread_exit(struct task_struct *thread_over, bool need_schedule)
{
    /* 要保证schedule在关中断情况下调用 */
    intr_disable();
    thread_over->status = TASK_DIED;

    /* 如果thread_over不是当前线程,就有可能还在就绪队列中,将其从中删除 */
    if (elem_find(&thread_ready_list, &thread_over->general_tag))
    {
        list_remove(&thread_over->general_tag);
    }
    if (thread_over->pgdir)
    { // 如是进程,回收进程的页表
        mfree_page(PF_KERNEL, thread_over->pgdir, 1);
    }

    /* 从all_thread_list中去掉此任务 */
    list_remove(&thread_over->all_list_tag);

    /* 回收pcb所在的页,主线程的pcb不在堆中,跨过 */
    if (thread_over != main_thread)
    {
        mfree_page(PF_KERNEL, thread_over, 1);
    }

    /* 归还pid */
    release_pid(thread_over->pid);

    /* 如果需要下一轮调度则主动调用schedule */
    if (need_schedule)
    {
        schedule();
        PANIC("thread_exit: should not be here\n");
    }
}

/* 比对任务的pid */
static bool pid_check(struct list_elem *pelem, int32_t pid)
{
    struct task_struct *pthread = elem2entry(struct task_struct, all_list_tag, pelem);
    if (pthread->pid == pid)
    {
        return true;
    }
    return false;
}

/* 根据pid找pcb,若找到则返回该pcb,否则返回NULL */
struct task_struct *pid2thread(int32_t pid)
{
    struct list_elem *pelem = list_traversal(&thread_all_list, pid_check, pid);
    if (pelem == NULL)
    {
        return NULL;
    }
    struct task_struct *thread = elem2entry(struct task_struct, all_list_tag, pelem);
    return thread;
}
//获取当前线程pcb指针
struct task_struct *running_thread()
{
    uint32_t esp;
    asm("mov %%esp,%0"
        : "=g"(esp));
    /*
        线程用的 0 级栈都是在自己的 pcb 中 （pcb占据一个自然页)
        因此 esp 的高 20 位就是当前 pcb 地址
    */
    return (struct task_struct *)(esp & 0xfffff000);
}

/*
不能通过call调用，详情见下面
*/
static void kernel_thread(thread_func *function, void *func_arg)
{
    //执行function 前要打开中断，避免时钟中断被屏蔽，无法调度其他线程
    intr_enable();
    function(func_arg);
}

/*
初始化线程栈 thread_stack, 将待执行的函数和参数放到 thread_stack 中相应的位置
*/
void thread_create(struct task_struct *pthread, thread_func function, void *func_arg)
{
    //pthread->self_kstack 已经在 (init_thread) 被指向到pcb顶端了

    //预留中断使用的栈空间，可见 thread.h
    pthread->self_kstack -= sizeof(struct intr_stack);

    //再留出线程栈空间，可见 thread.h
    pthread->self_kstack -= sizeof(struct thread_stack);

    struct thread_stack *kthread_stack = (struct thread_stack *)pthread->self_kstack;

    //线程创建时，eip 指向 kernel_thread
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;

    kthread_stack->ebp = kthread_stack->ebx =
        kthread_stack->esi = kthread_stack->edi = 0;
}

//初始化线程基本信息
void init_thread(struct task_struct *pthread, char *name, int prio)
{
    //pcb清0，一个pcb 一页
    memset(pthread, 0, sizeof(*pthread));
    pthread->pid=allocate_pid();
    strcpy(pthread->name, name);

    if (pthread == main_thread)
    {
        //main 函数也被封装成线程了，并且一直在运行
        pthread->status = TASK_RUNNING;
    }
    else
    {
        pthread->status = TASK_READY;
    }
    //self_kstack 是线程自己在内核态下使用的栈顶地址。 pthread 是pcb最低地址，加上一页大小，是pcb最高地址
    pthread->self_kstack = (uint32_t *)((uint32_t)pthread + PG_SIZE);
    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->elapsed_ticks = 0;
    pthread->pgdir = NULL;
    
    /* 预留标准输入输出 */
    pthread->fd_table[0] = 0;
    pthread->fd_table[1] = 1;
    pthread->fd_table[2] = 2;
    /* 其余的全置为-1 */
    uint8_t fd_idx = 3;
    while (fd_idx < MAX_FILES_OPEN_PER_PROC)
    {
        pthread->fd_table[fd_idx] = -1;
        fd_idx++;
    }
    pthread->cwd_inode_nr=0; //根目录为默认工作目录
    pthread->parent_pid = -1;     // -1表示没有父进程
    pthread->stack_magic = THREAD_MAGIC_NUM; //自定义魔数
}

/*
创建优先级为 prio 的线程，线程名为name，线程所执行的函数是 function(func_arg);
*/
struct task_struct *thread_start(char *name, int prio, thread_func function, void *func_arg)
{

    //pcb都位于内核空间，包括用户进程的pcb页属于内核空间
    struct task_struct *thread = get_kernel_pages(1); //thread 指向了pcb最低地址

    init_thread(thread, name, prio);
    thread_create(thread, function, func_arg);
    //确保之前不在队列中
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);

    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);

    return thread;
}

//kernel 中的 main 函数完善为主线程
static void make_main_thread(void)
{
    //loader.asm 中进入内核时， mov esp ,0xc009f00 预留了 pcb, 因此其 pcb 为 0xc009e00 (见memory.c)
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);

    //main 是当前线程，当前线程不应该在 ready_list 中。
    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

//实现任务调度
/*
    时钟中断时: (kernel.asm)


    在时钟中断或者其它时候，调用任务调度
    1. 获取当前运行的线程的 pcb:cur
    2. 判断条件，切换状态，重置时间片等
    3. 获取下一个要运行的线程的 pcb:next
    4. switch_to 当前 pcb 线程 到 next 线程 pcb
        具体步骤:
          0. 进入 switch_to 之前在创建线程时，压入的 kernel_thread 或者 运行 switch_to 的返回地址 eip
          1. 在当前 esp 下 压入 ebp,ebx,edi,esi (ABI 规则)
          2. 保存当前 esp 到 pcb 的 self_stack 指针
          3. 从 next 的 pcb 的 selft_stack 指针获取到 之前保存环境的 esp
          4. 加载esp 后，从当前栈 推出 esi,edi,ebx,ebp
          5. 此时 栈顶 esp 指向的是 之前压入的返回地址 （首次调用 : kernel_thread, 或者 switch_to 返回地址)
          6. 通过 ret 的形式，将返回地址加载到 当前 eip 寄存器，然后 cpu 继续从 eip 开始执行
                a: eip 为 kernel_thread 即首次调度，那么进入 kernel_thread, 调用 function
                b: eip 为 switch_to 返回地址。 可能是继续执行 schedule 然后继续执行中断处理函数，然后退出中断，然后继续执行该线程
    
    备注:
        1. 为啥用ret 调用而不是 call, call 需要 取数据，退栈，再压栈，不如 ret 一次性快
*/
void schedule()
{
    ASSERT(intr_get_status() == INTR_OFF);

    struct task_struct *cur = running_thread();
    if (cur->status == TASK_RUNNING) //如果是运行的时间片到了，加入到队尾
    {
        ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
        list_append(&thread_ready_list, &cur->general_tag);
        cur->ticks = cur->priority; //重置时间片为 priority
        cur->status = TASK_READY;   //ready状态
    }
    else
    {
        //线程需要某时间发生才能继续cpu运行，不需要加入队列，因为当前线程不在就绪队列中
    }

    /* 如果就绪队列中没有可运行的任务,就唤醒idle */
    if (list_empty(&thread_ready_list))
    {
        thread_unblock(idle_thread);
    }

    ASSERT(!list_empty(&thread_ready_list));
    thread_tag = NULL; //thread tag 清空
    thread_tag = list_pop(&thread_ready_list);
    //拿到 thread_tag 对应的 task_struct 指针
    struct task_struct *next = elem2entry(struct task_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;
    //激活任务页表等
    process_activate(next);
    switch_to(cur, next);
}
/* 以填充空格的方式输出buf */
static void pad_print(char *buf, int32_t buf_len, void *ptr, char format)
{
    memset(buf, 0, buf_len);
    uint8_t out_pad_0idx = 0;
    switch (format)
    {
    case 's':
        out_pad_0idx = sprintf(buf, "%s", ptr);
        break;
    case 'd':
        out_pad_0idx = sprintf(buf, "%d", *((int16_t *)ptr));
    case 'x':
        out_pad_0idx = sprintf(buf, "%x", *((uint32_t *)ptr));
    }
    while (out_pad_0idx < buf_len)
    { // 以空格填充
        buf[out_pad_0idx] = ' ';
        out_pad_0idx++;
    }
    sys_write(stdout_no, buf, buf_len - 1);
}

/* 用于在list_traversal函数中的回调函数,用于针对线程队列的处理 */
static bool elem2thread_info(struct list_elem *pelem, int arg UNUSED)
{
    struct task_struct *pthread = elem2entry(struct task_struct, all_list_tag, pelem);
    char out_pad[16] = {0};

    pad_print(out_pad, 16, &pthread->pid, 'd');

    if (pthread->parent_pid == -1)
    {
        pad_print(out_pad, 16, "NULL", 's');
    }
    else
    {
        pad_print(out_pad, 16, &pthread->parent_pid, 'd');
    }

    switch (pthread->status)
    {
    case 0:
        pad_print(out_pad, 16, "RUNNING", 's');
        break;
    case 1:
        pad_print(out_pad, 16, "READY", 's');
        break;
    case 2:
        pad_print(out_pad, 16, "BLOCKED", 's');
        break;
    case 3:
        pad_print(out_pad, 16, "WAITING", 's');
        break;
    case 4:
        pad_print(out_pad, 16, "HANGING", 's');
        break;
    case 5:
        pad_print(out_pad, 16, "DIED", 's');
    }
    pad_print(out_pad, 16, &pthread->elapsed_ticks, 'x');

    memset(out_pad, 0, 16);
    ASSERT(strlen(pthread->name) < 17);
    memcpy(out_pad, pthread->name, strlen(pthread->name));
    strcat(out_pad, "\n");
    sys_write(stdout_no, out_pad, strlen(out_pad));
    return false; // 此处返回false是为了迎合主调函数list_traversal,只有回调函数返回false时才会继续调用此函数
}

/* 打印任务列表 */
void sys_ps(void)
{
    char *ps_title = "PID            PPID           STAT           TICKS          COMMAND\n";
    sys_write(stdout_no, ps_title, strlen(ps_title));
    list_traversal(&thread_all_list, elem2thread_info, 0);
}

void thread_init()
{
    put_str("thread_init_start \n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    pid_pool_init();
    /* 先创建第一个用户进程:init */
    process_execute(init, "init"); // 放在第一个初始化,这是第一个进程,init进程的pid为1
    make_main_thread();
    idle_thread=thread_start("idle",10,idle,NULL);
    put_str("thread_init_done\n");
}

/*
 old thread_start
创建优先级为 prio 的线程，线程名为name，线程所执行的函数是 function(func_arg);
*/
/*struct task_struct *thread_start(char *name, int prio, thread_func function, void *func_arg)
{*/

//pcb都位于内核空间，包括用户进程的pcb页属于内核空间
/*struct task_struct *thread = get_kernel_pages(1); //thread 指向了pcb最低地址

    init_thread(thread, name, prio);
    thread_create(thread, function, func_arg);*/

/*
        thread->slef_stack 指向了线程栈最低处
        此时栈中数据为
        thread_stack{                  <-------- esp
            uint32_t ebp;   ---> = 初始化的 0
            uint32_t ebx;   ---> = 初始化的 0
            uint32_t edi;   ---> = 初始化的 0
            uint32_t esi;   ---> = 初始化的 0
            void (*eip)(thread_func *func, void *func_arg);  ---> = kernel_thread 地址
            void(*unused_retaddr);  ---> = 占位符 (当做 kernel_thread 的返回地址)
            thread_func *function;  ---> = 需要 kernel_thread 调用的方法
            void *func_arg;         ---> = fucntion 方法需要用的参数
         };
         pop ebp;
         pop ebx;
         pop edi;
         pop esi;
         此时 esp 指向了 eip
         通过 ret, 把 eip 指向的内容 (kernel_thread 地址 当做了返回地址) 
            即 没有通过 call 调用 kernel_thread, 而是通过 ret 调用了 kernel_thread
            所以在 call 之前，已经手动把栈里的数据填充了  (unused_retaddr, function,func_arg)
            因为 kernel_thread 是被 ret 调用的，所以它的栈中应该放一个伪造的占位返回地址。（以后不会通过它返回
            这样，才能定位到 function,func_arg 参数位置
    */

/*asm volatile("movl %0, %%esp;\
    pop %%ebp;\
    pop %%ebx;\
    pop %%edi;\
    pop %%esi;\
    ret" ::"g"(thread->self_kstack)
                 : "memory");
    return thread;*/
/*}*/

/*
当前线程将自己阻塞，标识其状态为 stat
*/
void thread_block(enum task_status stat)
{
    //stat 为 TASK_BLOCKED TASK_WAITING TASK_HANGING 才不会被调度
    ASSERT(stat == TASK_BLOCKED || stat == TASK_WAITING || stat == TASK_HANGING);

    enum intr_status old_status = intr_disable();
    struct task_struct *cur_thread = running_thread();
    cur_thread->status = stat;
    schedule();//当前线程换下处理器
    //当前线程被解除阻塞后，才继续运行下面的 intr_set_status
    intr_set_status(old_status);
}
//将线程 pthread 解除阻塞
void thread_unblock(struct task_struct *pthread)
{
    enum intr_status old_status = intr_disable();
    ASSERT(((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WAITING) || (pthread->status == TASK_HANGING)));
    if (pthread->status != TASK_READY)
    {
        ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));
        if (elem_find(&thread_ready_list, &pthread->general_tag))
        {
            PANIC("thread_ublock : blocked thread in ready_list \n");
        }
        list_push(&thread_ready_list, &pthread->general_tag);
        pthread->status = TASK_READY;
    }
    intr_set_status(old_status);
}
//主动让出cpu，让其他线程运行
void thread_yield(void)
{
    struct task_struct *cur = running_thread();
    enum intr_status old_status = intr_disable();
    ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
    list_append(&thread_ready_list, &cur->general_tag);
    cur->status = TASK_READY;
    schedule();
    intr_set_status(old_status);
}
//系统空闲时运行的线程
static void idle(void *arg UNUSED)
{
    while (1)
    {
        thread_block(TASK_BLOCKED);//阻塞字节，等待schedule调度
        asm volatile("sti; hlt" ::
                         : "memory"); //停机，只响应硬件中断
    }
}