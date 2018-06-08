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
static void test_string(void);
static void test_assert(void);
static void test_bitmap(void);
static void test_memory(void);
static void k_thread_a(void *);
static void k_thread_b(void *);
static void u_prog_a(void);
static void u_prog_b(void);
static int test_var_a = 0, test_var_b = 0;
static int proga_pid, progb_pid = 0;
static void a_end();
static void b_end();
static void read_file();
static void createdir();
static void opendir();
static void readdir();
static void deletedir();
static void chdir();
void init();

void main(void)
{
    clean_screen();

    put_str("I am kerneal\n");
    init_all();
    /*************    写入应用程序    *************/
    //   uint32_t file_size = 6076;
    //   uint32_t sec_cnt = DIV_ROUND_UP(file_size, 512);
    //   struct disk* sda = &channels[0].devices[0];
    //   void* prog_buf = sys_malloc(file_size);
    //   ide_read(sda, 300, prog_buf, sec_cnt);
    //   int32_t fd = sys_open("/prog_no_arg", O_CREAT|O_RDWR);
    //   if (fd != -1) {
    //      if(sys_write(fd, prog_buf, file_size) == -1) {
    // 	 printk("file write error!\n");
    // 	 while(1);
    //      }
    //   }
    /*************    写入应用程序结束   *************/
  //cls_screen();
  printf("[rabbit@localhost %s]$ ", "/");
    //process_execute(u_prog_a, "user_proga");
    // process_execute(u_prog_b, "user_progb");
    // console_put_str("main pid:0x");
    // console_put_int(getpid());
    // console_put_char('\n');
    // stat();
    //thread_start("k_thread_a,", 31, k_thread_a, "A");
    //thread_start("k_thread_b,", 31, k_thread_b, "B");

    while (1)
    {
        //console_put_str("Main\n");
    }
}


/* init进程 */
void init(void)
{

    uint32_t ret_pid = fork();
    if (ret_pid)
    {
      
       while(1){

           /* code */
       }
    }
    else
    {
        
        my_shell();
    }
    while (1)
        ;
}
static void stat()
{
    /********  测试代码  ********/
    struct stat obj_stat;
    sys_stat("/", &obj_stat);
    printf("/`s info\n   i_no:%d\n   size:%d\n   filetype:%s\n",
           obj_stat.st_ino, obj_stat.st_size,
           obj_stat.st_filetype == 2 ? "directory" : "regular");
    sys_stat("/dir1", &obj_stat);
    printf("/dir1`s info\n   i_no:%d\n   size:%d\n   filetype:%s\n",
           obj_stat.st_ino, obj_stat.st_size,
           obj_stat.st_filetype == 2 ? "directory" : "regular");
    /********  测试代码  ********/
}
static void chdir()
{
    /********  测试代码  ********/
    char cwd_buf[32] = {0};
    sys_getcwd(cwd_buf, 32);
    printf("cwd:%s\n", cwd_buf);
    //printf("create /dir1\n", sys_mkdir("/dir1") == 0 ? "done" : "fail");
    sys_chdir("/dir1");
    printf("change cwd now\n");
    sys_getcwd(cwd_buf, 32);
    printf("cwd:%s\n", cwd_buf);
    /********  测试代码  ********/
}
static void deletedir()
{
    /********  测试代码  ********/
    printf("/dir1 content before delete /dir1/subdir1:\n");
    struct dir *dir = sys_opendir("/dir1/");
    char *type = NULL;
    struct dir_entry *dir_e = NULL;
    while ((dir_e = sys_readdir(dir)))
    {
        if (dir_e->f_type == FT_REGULAR)
        {
            type = "regular";
        }
        else
        {
            type = "directory";
        }
        printf("      %s   %s\n", type, dir_e->filename);
    }
    printf("try to delete nonempty directory /dir1/subdir1\n");
    if (sys_rmdir("/dir1/subdir1") == -1)
    {
        printf("sys_rmdir: /dir1/subdir1 delete fail!\n");
    }

    printf("try to delete /dir1/subdir1/file2\n");
    if (sys_rmdir("/dir1/subdir1/file2") == -1)
    {
        printf("sys_rmdir: /dir1/subdir1/file2 delete fail!\n");
    }
    if (sys_unlink("/dir1/subdir1/file2") == 0)
    {
        printf("sys_unlink: /dir1/subdir1/file2 delete done\n");
    }

    printf("try to delete directory /dir1/subdir1 again\n");
    if (sys_rmdir("/dir1/subdir1") == 0)
    {
        printf("/dir1/subdir1 delete done!\n");
    }

    printf("/dir1 content after delete /dir1/subdir1:\n");
    sys_rewinddir(dir);
    while ((dir_e = sys_readdir(dir)))
    {
        if (dir_e->f_type == FT_REGULAR)
        {
            type = "regular";
        }
        else
        {
            type = "directory";
        }
        printf("      %s   %s\n", type, dir_e->filename);
    }

    /********  测试代码  ********/
}
static void readdir()
{
    /********  测试代码  ********/
    struct dir *p_dir = sys_opendir("/dir1/subdir1");
    if (p_dir)
    {
        printf("/dir1/subdir1 open done!\ncontent:\n");
        char *type = NULL;
        struct dir_entry *dir_e = NULL;
        while ((dir_e = sys_readdir(p_dir)))
        {
            if (dir_e->f_type == FT_REGULAR)
            {
                type = "regular";
            }
            else
            {
                type = "directory";
            }
            printf("      %s   %s\n", type, dir_e->filename);
        }
        if (sys_closedir(p_dir) == 0)
        {
            printf("/dir1/subdir1 close done!\n");
        }
        else
        {
            printf("/dir1/subdir1 close fail!\n");
        }
    }
    else
    {
        printf("/dir1/subdir1 open fail!\n");
    }
    /********  测试代码  ********/
}
static void createdir()
{
    printf("/dir1/subdir1 create %s!\n", sys_mkdir("/dir1/subdir1") == 0 ? "done" : "fail");
    printf("/dir1 create %s!\n", sys_mkdir("/dir1") == 0 ? "done" : "fail");
    printf("now, /dir1/subdir1 create %s!\n", sys_mkdir("/dir1/subdir1") == 0 ? "done" : "fail");
    int fd = sys_open("/dir1/subdir1/file2", O_CREAT | O_RDWR);
    if (fd != -1)
    {
        printf("/dir1/subdir1/file2 create done!\n");
        sys_write(fd, "Catch me if you can!\n", 21);
        sys_lseek(fd, 0, SEEK_SET);
        char buf[32] = {0};
        sys_read(fd, buf, 21);
        printf("/dir1/subdir1/file2 says:\n%s", buf);
        sys_close(fd);
    }
}
static void opendir()
{
    struct dir *p_dir = sys_opendir("/dir1/subdir1");
    if (p_dir)
    {
        printf("/dir1/subdir1 open done!\n");
        if (sys_closedir(p_dir) == 0)
        {
            printf("/dir1/subdir1 close done!\n");
        }
        else
        {
            printf("/dir1/subdir1 close fail!\n");
        }
    }
    else
    {
        printf("/dir1/subdir1 open fail!\n");
    }
}
static void read_file()
{
    uint32_t fd = sys_open("/f1", O_RDWR);
    printf("open /file1, fd:%d\n", fd);
    char buf[64] = {0};
    int read_bytes = sys_read(fd, buf, 18);
    printf("1_ read %d bytes:\n%s\n", read_bytes, buf);

    memset(buf, 0, 64);
    read_bytes = sys_read(fd, buf, 6);
    printf("2_ read %d bytes:\n%s", read_bytes, buf);

    memset(buf, 0, 64);
    read_bytes = sys_read(fd, buf, 6);
    printf("3_ read %d bytes:\n%s", read_bytes, buf);

    printf("________  SEEK_SET 0  ________\n");
    sys_lseek(fd, 0, SEEK_SET);
    memset(buf, 0, 64);
    read_bytes = sys_read(fd, buf, 24);
    printf("4_ read %d bytes:\n%s", read_bytes, buf);

    sys_close(fd);
}
static void a_end()
{

    while (1)
    {
        /* code */
    }
}
static void b_end2()
{
}
static void b_end()
{
}
static void k_thread_b(void *arg)
{

    console_put_str(" thread_b end\n");
    while (1)
        ;
}
static void k_thread_a(void *arg)
{
    printk("%s,%d,0x%x,%c", "hello", 100, 0xff, '\n');

    while (1)
    {
        /* code */
    }
}
static void malloc_me()
{
    int size = 1;
    void *ptrs[30];
    int cnt = 0;
    while (size < 2049)
    {
        void *p1 = malloc(size);
        ptrs[cnt++] = p1;
        printf("size=%d addr=0x%x\n", size, p1);
        size *= 2;
    }

    while (cnt--)
    {
        printf("free addr=0x%x\n", ptrs[cnt]);
        free(ptrs[cnt]);
    }
}
static void u_prog_a()
{

    // malloc_me();
    clear();
    putchar('A');
    while (1)
    {
    }
}
static void u_prog_b()
{
    progb_pid = getpid();
    while (1)
    {
    }
}
static void test_memory(void)
{
    void *addr = get_kernel_pages(3);
    put_str("get_kernel_page start vaddr is 0x");
    put_int((uint32_t)addr);
    put_char('\n');
}

static void test_string(void)
{
    put_str("test_string\n");
    char str1[100];
    char *go = "hello";
    strcpy(str1, go);
    uint32_t len = strlen(str1);
    put_str(str1);
    put_int(len);
    put_str(go);
}
static void test_assert(void)
{
    ASSERT(1 == 2);
}

static void test_bitmap(void)
{
    uint8_t stack_mem[1024];
    struct bitmap btmp_s;
    struct bitmap *btmp = &btmp_s;
    btmp->bits = stack_mem;
    btmp->btmp_bytes_len = 512;
    bitmap_init(btmp);
    int start = bitmap_scan(btmp, 32);
    if (start >= 0)
    {
        bitmap_set(btmp, 0, 1);
        bitmap_set(btmp, 1, 1);
        bitmap_set(btmp, 2, 1);
        bitmap_set(btmp, 15 * 8, 1);
        bitmap_set(btmp, 31 * 8, 1);
    }
    _test_print_bitmap(btmp, 32);
    int s1 = bitmap_scan(btmp, 30);
    int s2 = bitmap_scan(btmp, 32 * 8);
    put_str("30 free: 0x");
    put_int(s1);
    put_str("\n256 free: 0x");
    put_int(s2);
}
