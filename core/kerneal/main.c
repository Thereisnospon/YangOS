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

    process_execute(u_prog_a, "user_proga");
    // process_execute(u_prog_b, "user_progb");
  
    intr_enable();
    // console_put_str("main pid:0x");
    // console_put_int(getpid());
    // console_put_char('\n');

  //  thread_start("k_thread_a,", 31, k_thread_a, "A");
   //  thread_start("k_thread_b,", 31, k_thread_b, "B");
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
  
    console_put_str(" thread_b start\n");
    console_put_str(" thread_b end\n");
    
    void *arg1 = sys_malloc(30);
    b_end();
    sys_free(arg1);
    b_end2();
    while (1){
        ;
    }     
}
void k_thread_a(void *arg)
{
    char *para = arg;
    void *addr1;
    void *addr2;
    void *addr3;
    void *addr4;
    void *addr5;
    void *addr6;
    void *addr7;
    console_put_str(" thread_a start\n");
    int max = 1000;
    while (max-- > 0)
    {
        console_put_int(max);
        int size = 128;
        addr1 = sys_malloc(size);
        size *= 2;
        addr2 = sys_malloc(size);
        size *= 2;
        addr3 = sys_malloc(size);
        sys_free(addr1);
        addr4 = sys_malloc(size);
        size *= 2;
        size *= 2;
        size *= 2;
        size *= 2;
        size *= 2;
        size *= 2;
        size *= 2;
        addr5 = sys_malloc(size);
        addr6 = sys_malloc(size);
        sys_free(addr5);
        size *= 2;
        addr7 = sys_malloc(size);
        sys_free(addr6);
        sys_free(addr7);
        sys_free(addr2);
        sys_free(addr3);
        sys_free(addr4);
      
    }
    console_put_str(" thread_a end\n");
    a_end();
    while (1)
        ;
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
