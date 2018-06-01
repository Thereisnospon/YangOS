#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"
#include "debug.h"
#include "list.h"
#include "print.h"
#include "interrupt.h"
#include "process.h"
#include "sync.h"

#define PG_SIZE 4096

struct task_struct *main_thread;     //主线程pcb
struct list thread_ready_list;       //就绪队列
struct list thread_all_list;         //所有任务队列
static struct list_elem *thread_tag; //用于保存队列中的线程节点
struct lock pid_lock;
extern void switch_to(struct task_struct *cur, struct task_struct *next);
//分配pid
static pid_t allocate_pid(void)
{
    static pid_t next_pid = 0;
    lock_acquire(&pid_lock);
    next_pid++;
    lock_release(&pid_lock);
    return next_pid;
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
    pthread->elasped_ticks = 0;
    pthread->pgdir = NULL;
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

void thread_init()
{
    put_str("thread_init_start \n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    lock_init(&pid_lock);
    make_main_thread();
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