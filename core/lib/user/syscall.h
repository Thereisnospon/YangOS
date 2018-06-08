#ifndef __LIB_USER_SYSCALL_H
#define __LIB_USER_SYSCALL_H
#include "stdint.h"
enum SYSCALL_NR
{
    SYS_GETPID,
    SYS_WRITE,
    SYS_MALLOC,
    SYS_FREE,
    SYS_FORK,
    SYS_READ,
    SYS_PUTCHAR,
    SYS_CLEAR,
    SYS_GETCWD,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_LSEEK,
    SYS_UNLINK,
    SYS_MKDIR,
    SYS_OPENDIR,
    SYS_CLOSEDIR,
    SYS_CHDIR,
    SYS_RMDIR,
    SYS_READDIR,
    SYS_REWINDDIR,
    SYS_STAT,
    SYS_PS,
    SYS_EXECV,
    SYS_EXIT,
    SYS_WAIT,
    SYS_PIPE
};
/* 返回当前任务pid */
uint32_t getpid();
/* 把buf中count个字符写入文件描述符fd */
uint32_t write(int32_t fd, const void *buf, uint32_t count);
/* 申请size字节大小的内存,并返回结果 */
void *malloc(uint32_t size);
/* 释放ptr指向的内存 */
void free(void *ptr);
/* 派生子进程,返回子进程pid */
int16_t fork(void);
/* 从文件描述符fd中读取count个字节到buf */
int32_t read(int32_t fd, void *buf, uint32_t count);
/* 输出一个字符 */
void putchar(char char_asci);
/* 清空屏幕 */
void clear(void);
/* 获取当前工作目录 */
char *getcwd(char *buf, uint32_t size);
/* 以flag方式打开文件pathname */
int32_t open(char *pathname, uint8_t flag);
/* 关闭文件fd */
int32_t close(int32_t fd);
/* 设置文件偏移量 */
int32_t lseek(int32_t fd, int32_t offset, uint8_t whence);
/* 删除文件pathname */
int32_t unlink(const char *pathname);
/* 创建目录pathname */
int32_t mkdir(const char *pathname);
/* 打开目录name */
struct dir *opendir(const char *name);
/* 关闭目录dir */
int32_t closedir(struct dir *dir);
/* 删除目录pathname */
int32_t rmdir(const char *pathname);
/* 读取目录dir */
struct dir_entry *readdir(struct dir *dir);
/* 回归目录指针 */
void rewinddir(struct dir *dir);

/* 改变工作目录为path */
int32_t chdir(const char *path);
/* 显示任务列表 */
void ps(void);
int execv(const char *pathname, char **argv);
/* 以状态status退出 */
void exit(int32_t status);
/* 等待子进程,子进程状态存储到status */
int16_t wait(int32_t *status);
/* 生成管道,pipefd[0]负责读入管道,pipefd[1]负责写入管道 */
int32_t pipe(int32_t pipefd[2]);

#endif
