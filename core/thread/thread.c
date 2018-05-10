#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"
#define PG_SIZE 4096


/*
不能通过call调用，详情见下面
*/
static void kernel_thread(thread_func *function, void *func_arg)
{
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
    strcpy(pthread->name, name);
    pthread->status = TASK_RUNNING;
    pthread->priority = prio;
    //self_kstack 是线程自己在内核态下使用的栈顶地址。 pthread 是pcb最低地址，加上一页大小，是pcb最高地址
    pthread->self_kstack = (uint32_t *)((uint32_t)pthread + PG_SIZE);
    pthread->stack_magic = 0x19870916; //自定义魔数
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

    asm volatile("movl %0, %%esp;\
    pop %%ebp;\
    pop %%ebx;\
    pop %%edi;\
    pop %%esi;\
    ret" ::"g"(thread->self_kstack)
                 : "memory");
    return thread;
}
