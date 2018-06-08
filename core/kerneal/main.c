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
void init();
void try_write(char *name, int sector, uint32_t file_size);
void run4();
void run3();
void run5();
void main(void)
{
    clean_screen();
    put_str("I am kerneal\n");
    init_all();
    /*************    写入应用程序    *************/
    // run4();
    run5();
    /*************    写入应用程序结束   *************/
    // cls_screen();
    printf("[rabbit@localhost %s]$ ", "/");
    thread_exit(running_thread(), true);
    return 0;
}
void run5(){
    try_write("/prog_pipe",300,6800);
}
void run3()
{
    try_write("/prog_arg", 300, 6816); //需要先make prog_arg 写入到os.img
}
void run4()
{
    try_write("/cat", 300, 6816); //需要先make cat 写入到os.img
    try_write("/f1", 350, 512);   //在make cat 时，已经写入到os.img 了
}
void try_write(char *name, int sector, uint32_t file_size)
{
    uint32_t sec_cnt = DIV_ROUND_UP(file_size, 512);
    struct disk *sda = &channels[0].devices[0];
    void *prog_buf = sys_malloc(file_size);
    ide_read(sda, sector, prog_buf, sec_cnt);
    int32_t fd = sys_open(name, O_CREAT | O_RDWR);
    if (fd != -1)
    {
        if (sys_write(fd, prog_buf, file_size) == -1)
        {
            printk("file write error!\n");
            while (1)
                ;
        }
    }
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
