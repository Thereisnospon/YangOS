#include "print.h"
#include "init.h"
#include "debug.h"
#include "string.h"
#include "bitmap.h"
#include "memory.h"
#include "console.h"
#include "ioqueue.h"
#include "keyboard.h"
#include "interrupt.h"
#include "process.h"
#include "syscall.h"
#include "syscall-init.h"
#include "stdio.h"
#include "stdio-kernel.h"
#include "fs.h"
#include "dir.h"
#include "shell.h"
#include "buildin_cmd.h"
void init();
void main(void)
{
    clean_screen();
    put_str("I am kerneal\n");
    init_all();
    /*************    写入应用程序    *************/
    uprog_init();
    /*************    写入应用程序结束   *************/
    // cls_screen();
    printf("[rabbit@localhost %s]$ ", "/");
    thread_exit(running_thread(), true);
    return 0;
}
/* init进程 */
void init(void)
{
    uint32_t ret_pid = fork();
    if (ret_pid)
    { // 父进程
        int status;
        int child_pid;
        /* init在此处不停的回收僵尸进程 */
        while (1)
        {
            child_pid = wait(&status);
            printf("I`m init, My pid is 1, I recieve a child, It`s pid is %d, status is %d\n", child_pid, status);
        }
    }
    else
    { // 子进程
        my_shell();
    }
    PANIC("init: should not be here");
}
