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

void test_string(void);
void test_assert(void);
void test_bitmap(void);
void test_memory(void);
void k_thread_a(void *);
void k_thread_b(void *);

void main(void)
{
    clean_screen();

    put_str("I am kerneal\n");
    init_all();
    //test_string();
    // test_bitmap();
    // test_assert();
    // test_memory();
    thread_start("k_thread_a,", 31, k_thread_a, "A");
    thread_start("k_thread_b", 8, k_thread_b, "B");
    intr_enable();
    //ASSERT(1 == 2);
    while (1)
    {
        //console_put_str("Main\n");
    }
}
void k_thread_b(void *arg)
{
    char *para = arg;
    while (1)
    {
      
        enum intr_status old_status = intr_disable();
        if (!ioq_empty(&kbd_buf))
        {
            console_put_str(arg);
            char byte = ioq_getchar(&kbd_buf);
            console_put_char(byte);
        }
        intr_set_status(old_status);
    }
}
void k_thread_a(void *arg)
{
    char *para = arg;
    while (1)
    {
        
        enum intr_status old_status = intr_disable();
        if (!ioq_empty(&kbd_buf))
        {
            console_put_str(arg);
            char byte = ioq_getchar(&kbd_buf);
            console_put_char(byte);
        }
        intr_set_status(old_status);
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
