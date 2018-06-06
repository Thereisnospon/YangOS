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

void test_string(void);
void test_assert(void);
void test_bitmap(void);
void test_memory(void);
void k_thread_a(void *);
void k_thread_b(void *);
void u_prog_a(void);
void u_prog_b(void);
int test_var_a = 0, test_var_b = 0;
int proga_pid,progb_pid=0;
void a_end();
void b_end();
void main(void)
{
    clean_screen();

    put_str("I am kerneal\n");
    init_all();

   // process_execute(u_prog_a, "user_proga");
    // process_execute(u_prog_b, "user_progb");
  
    intr_enable();
    // console_put_str("main pid:0x");
    // console_put_int(getpid());
    // console_put_char('\n');

 //thread_start("k_thread_a,", 31, k_thread_a, "A");
   //thread_start("k_thread_b,", 31, k_thread_b, "B");
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
    while (1)
    {
        //console_put_str("Main\n");
    }
}
void a_end(){
    
    while(1){
        /* code */
    }
    
}
void b_end2(){

}
void b_end(){
    
}
void k_thread_b(void *arg)
{

   
    console_put_str(" thread_b end\n");
    while (1)
        ;
}
void k_thread_a(void *arg)
{
   printk("%s,%d,0x%x,%c","hello",100,0xff,'\n');

   while(1){
       /* code */
   }
   
}

void u_prog_a()
{
    int size=1;
    void*ptrs[30];
    int cnt=0;
    while(size<2049){
        void *p1=malloc(size);
        ptrs[cnt++]=p1;
        printf("size=%d addr=0x%x\n",size,p1);
        size*=2;
    }

    while(cnt--){
        printf("free addr=0x%x\n", ptrs[cnt]);
        free(ptrs[cnt]);
     
    }
    
    

    while (1)
    {

   }
}
void u_prog_b()
{
    progb_pid = getpid();
    while (1)
    {
    }
}
void test_memory(void)
{
    void *addr = get_kernel_pages(3);
    put_str("get_kernel_page start vaddr is 0x");
    put_int((uint32_t)addr);
    put_char('\n');
}

void test_string(void)
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
void test_assert(void)
{
    ASSERT(1 == 2);
}

void test_bitmap(void)
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
