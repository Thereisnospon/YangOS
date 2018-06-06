#include "timer.h"
#include "io.h"
#include "print.h"
#include "thread.h"
#include "debug.h"
#define IRQ0_FREQUENCY   100 //100hz
#define INPUT_FREQUENCY 1193180//计数器频率为 1.19318mhz, 即1秒发生 1193180 次脉冲
#define COUNTER0_VALUE INPUT_FREQUENCY/IRQ0_FREQUENCY// 1 秒计数器脉冲数/ 100 频数。
                                                    // 即发生一次时钟中断的 脉冲计数
#define COUNTER0_PORT 0x40 // 计数器0 的端口号
#define COUNTER0_NO 0    //计数器0
#define COUNTER_MODE 2  //方式2 比率发生器
#define READ_WRITE_LATCH 3 //先读写低8位，再读写高8位
#define PIT_CONTROL_PORT 0x43
#define mil_seconds_per_intr (1000 / IRQ0_FREQUENCY)

uint32_t ticks; //内核自中断以来总共的滴答数

/*----------------------------------------
|         8253控制字                      |
------------------------------------------
↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓
8253控制字 :
   
    |    7|    6|    5|    4|   3|   2|   1|    0|
    | SC1 | SC0 | RW1 | RW0 | M2 | M1 | M0 | BCD |

SC1-SC0 选择计数器位： Selecter Counter, 或者叫选择通道  Selector Channel
8253 有三个独立计数器，有独自控制模式，但是他们通过同一个控制字寄存器写入
SCW1 SCW0
  00 | 选择计数器0
  01 | 选择计数器1
  10 | 选择计数器2
  11 | 未定义

 
 RW1,RW0: 读/写 锁存操作位，即Read/Write/Latch
 用来设置待操作计数器的读写及锁存方式
 RW1 RW0 |
  00     | 锁存数据，供cpu读
  01     | 只读写低字节
  10     | 只读写高字节
  11     | 先读写低字节，后读写高字节

M2,M0: 工作方式选择位
    000 | 方式0 计数结束中断方式
    001 | 方式1 硬件可重触发单隐方式
    x10 | 方式2 比率发生器
    x11 | 方式3 方波发生器
    100 | 方式4 软件触发选通
    101 | 方式5 硬件触发选通

BCD 数制位:
    1, BCD 码。 0x1234 表示十进制 1234. 范围 0~9999
    0, 二进制。 0x1234 表示十进制 4660，范围 0~0xFFFF 即十进制 0~65535
↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑
*/

/*
把操作的计数器 counter_no ，读写锁属性 rwl, 计数器模式 counter_mode
 写入模式控制寄存器并赋予初始值 counter_value
*/
static void frequency_set(uint8_t counter_port,\
                          uint8_t counter_no,
                          uint8_t rwl, \
                          uint8_t counter_mode,\
                          uint16_t counter_value
){

    /*
    |    7|    6|    5|    4|   3|   2|   1|    0|
    | SC1 | SC0 | RW1 | RW0 | M2 | M1 | M0 | BCD |

    counter_no<<6 得到 SC1 SC0
    rwl << 4 得到 RW1 RW0 
    counter_mode <<1 得到 M2 M1 M0
    BCD 设置为 0 二进制模式
    */
    outb(PIT_CONTROL_PORT,\
    (uint8_t)(counter_no<<6 | rwl <<4 | counter_mode <<1) );

    //低8位
    outb(counter_port,(uint8_t)counter_value);
    //高8位
    outb(counter_port,(uint8_t)counter_value>>8);
}

static void intr_timer_handler(void)
{
    struct task_struct *cur_thread = running_thread();
    ASSERT(cur_thread->stack_magic == THREAD_MAGIC_NUM);//检查栈是否溢出
    cur_thread->elapsed_ticks++;                        //记录此线程占用的cpu时间
    ticks++;//内核第一次中断开始使用的滴答数
    if (cur_thread->ticks == 0)//时间片用完就开始调度新的进程上cpu
    {
        schedule();
    }
    else
    {
        //时间片-1
        cur_thread->ticks--;
    }
}
//以tick为单位进行休眠
static void ticks_to_sleep(uint32_t sleep_ticks)
{
    uint32_t start_tick = ticks;
    //间隔tick不够，就让出cpu
    while (ticks - start_tick < sleep_ticks)
    {
        thread_yield();
    }
}
//以毫秒为单位sleep
void mtime_sleep(uint32_t m_seconds)
{
    uint32_t sleep_ticks = DIV_ROUND_UP(m_seconds, mil_seconds_per_intr);
    ASSERT(sleep_ticks > 0);
    ticks_to_sleep(sleep_ticks);
}
void timer_init(){
    put_str("timer_init start\n");
    frequency_set(COUNTER0_PORT,\
                  COUNTER0_NO,\
                  READ_WRITE_LATCH,\
                  COUNTER_MODE,\
                  COUNTER0_VALUE
                  );
    register_handler(0x20,intr_timer_handler);
    put_str("timer_init done \n");
}