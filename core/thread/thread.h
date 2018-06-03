#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "stdint.h"
#include "list.h"
#include "memory.h"
//自定义通用函数类型
typedef void thread_func(void *);
typedef int16_t pid_t;
#define THREAD_MAGIC_NUM 0x19870916

void schedule();
struct task_struct *running_thread();
extern struct list thread_ready_list;
extern struct list thread_all_list;
//进程或者线程状态
enum task_status
{
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
};

/* 中断栈 intr_stack
此结构用于中断发生时保护程序(线程或者进程)的上下文环境
进程或线程被外部中断或者软中断打断时，会按照此结构压入上下文
寄存器，intr_exit 中的出栈操作是该结构的逆操作
此栈在线程自己的内核栈位置固定，在页的最顶端
*/

struct intr_stack
{
    uint32_t vec_no; //kerneal.asm 宏 VECTOR 中 push %1 压入的中断号
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    //以下由cpu从低特权级进入高特权级时压入
    uint32_t err_code; //error_code 会被压在 eip 之后
    void (*eip)(void);
    uint32_t cs;
    uint32_t eflags;
    void *esp;
    uint32_t ss;
};

/*线程栈 thread_stack
线程自己的栈，用于存储线程中待执行的函数
此结构在线程自己的内核栈中位置不固定
仅用在 switch_to 时保存线程环境
实际位置取决于实际运行情况
*/
struct thread_stack
{
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    //线程第一次执行时，eip 指向待调用的函数 kernel_thread 其他时候，eip 指向 switch_to 的返回地址
    void (*eip)(thread_func *func, void *func_arg);

    /* 以下在第一次被cpu调度时使用*/

    //参数 unused_retaddr 只为占位置充数为返回地址
    void(*unused_retaddr);
    thread_func *function; //kernel_thread 调用的函数
    void *func_arg;        //kernel_thread 调用的函数所需要的参数
};

//进程或者线程的pcb，程序控制块
struct task_struct
{
    uint32_t *self_kstack; // 各内核线程用自己的内核栈
    pid_t pid;
    enum task_status status;
    char name[16];
    uint8_t priority;              //线程优先级
    uint8_t ticks;                 // 每次在处理器上执行的时间滴答数
    uint32_t elasped_ticks;        //自从任务在cpu运行后，至今占用了多少滴答数
    struct list_elem general_tag;  //用于线程在一般的队列中的节点
    struct list_elem all_list_tag; //用于线程队列 thread_all_list 中的节点
    uint32_t *pgdir;               //进程自己的页表的虚拟地址
    struct virtual_addr userprog_vaddr;//用户进程的虚拟地址
    struct mem_block_desc u_block_desc[DESC_CNT]; //用户进程内存块描述符
    uint32_t stack_magic;          //栈的边界标记，用于检测栈的溢出
};

#endif

/*
struct task_struct
{
    uint32_t *self_kstack; 
        struct thread_stack
            {
                uint32_t ebp;
                uint32_t ebx;
                uint32_t edi;
                uint32_t esi;
                void (*eip)(thread_func *func, void *func_arg);
                void(*unused_retaddr);
                thread_func *function; 
                void *func_arg;        
            };
    enum task_status status;
    char name[16];
    uint8_t priority;             
    uint8_t ticks;                
    uint32_t elasped_ticks;        
    struct list_elem general_tag;  
    struct list_elem all_list_tag; 
    uint32_t *pgdir;              
    uint32_t stack_magic;
    };
*/

// ;       <-------------------> pcb 顶端 
// ;    ↓←←←←←| self_stack  |
// ;    ↓   <--+-------------+-->
// ;    ↓     |             |
// ;    ↓     |             |
// ;    ↓     |             |
// ;    ↓→→→→-+-------------+-->
// ;          |thread_stack |
// ;          --           --
// ;          |    ebp      |
// ;          --           --
// ;          |    ebx      |
// ;          --           --
// ;          |    edi      |
// ;          --           --
// ;          |    esi      |
// ;          --           --
// ;          |    eip      |
// ;          --           --
// ;          |   unused    |
// ;          --           --
// ;          |   function  |
// ;          --           --
// ;          |   func_arg  |
// ;        <--+-------------+-->
// ;          |             | 初始情况下，此栈在线程自己的内核栈中固定，在 pcb 页顶端，每次进入中断时并不一样
// ;          | intr_stack  | 如果进入中断不涉及 特权级变换，位置在 esp 之下，否则在 tss 中获取
// ;          |             |
// ;       <--+-------------+--> pcb 顶端

// ; <------------------->
// ;    |             |
// ;    |             |
// ;    |             |
// ; <--+-------------+-->