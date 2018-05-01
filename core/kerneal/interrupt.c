#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"
#include "print.h"

#define IDT_DESC_CNT 0x21 //目前支持的中断数

#define PIC_M_CTRL 0x20 //主片控制端口 0x20
#define PIC_M_DATA 0x21 //主片数据端口 0x21
#define PIC_S_CTRL 0xa0 //从片控制端口 0xa0
#define PIC_S_DATA 0xa1 //从片数据端口 0xa1

//中断门描述符结构体
struct gate_desc
{
    uint16_t func_offset_low_word;
    uint16_t selector;
    uint8_t dcount; // 双字计数字段，门描述符第 4 字节
                    // 固定值，不用考虑

    uint8_t attribute;
    uint16_t func_offset_high_word;
};

/*
    typedf void* intr_handler
    idtr_handler 是空指针类型。没有具体类型，仅仅表示地址。因为 int_entry_table 中元素是普通地址类型。
*/

static void make_idt_desc(struct gate_desc *p_gdesc, uint8_t attr, intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT]; // idt 是中断描述符表
                                           // 本质上是个中断门描述符数组

// intr_handler 是指针类型 ， intr_entry_table 是需要引入的符号名 且该符号是个数组
extern intr_handler intr_entry_table[IDT_DESC_CNT]; //声明定义在 kerneal.asm 中的中断处理函数入口数组

/*----------------------------------------
|          ICW1                          |
------------------------------------------
↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓

ICW1:
 | 7 | 6 | 5 | 4 |     3|    2|     1|    0|
 | 0 | 0 | 0 | 1 | LTIM | ADI | SNGL | IC4 |
    - IC4 表示是否要写入 ICW4 (ICW 初始不一定需要初始化 ICW4) x86 系统 IC4 必须为 1
    - SNGL 表示 single, 如果为 1 表示单片，0 表示级联
    - ADI 表示 call address interval 用来设置 8085 调用时间间隔，x86 不需要配置
    - LTIM 表示 level/edge triggered mode 设置中断检测方式。 0 表示边缘触发，1 表示电平触发
    - 第 4 位 1 固定。 是 ICW1 标记
    - 5~7 位用在 8085 处理器

↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑
*/

/*----------------------------------------
|          ICW2                          |
------------------------------------------
↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓
ICW2: 用来设置起始中断向量号  (只需要填写高5位，低3位不用管)
|  7 |  6 |  5 |  4 |  3 |   2 |   1 |   0 |
| T7 | T6 | T5 | T4 | T3 | ID2 | ID1 | ID0 |

↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑
*/

/*----------------------------------------
|          ICW3                          |
------------------------------------------
↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓
ICW3:仅在级联的方式下才需要（如果ICW1 的 SNGL=0),用来设置主片和从片用哪个IRQ接口互联

主片ICW3:
|  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0|
| S7 | S6 | S5 | S4 | S3 | S2 | S1 | S0|

主片的 ICW3 哪一位为1，表示这位对应的 IRQ 接口连接从片， 为 0 表示连接外部设备. 
比如,主片 IRQ2 和 IRQ5 接从片， 主片 ICW3 为 00100100
例，主片ICW3:
    |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0|
    | S7 | S6 | S5 | S4 | S3 | S2 | S1 | S0|
    |  0 |  0 |  1 |  0 |  0 |  1 |  0 |  0|

从片ICW3:
|  7|  6|  5|  4|  3|    2|    1|    0|
| 0 | 0 | 0 | 0 | 0 | ID2 | ID1 | ID0 |
从片 ICW3 设置 主片用于接 该从片 的 IRQ 端口。


例如: 从片 A 接了主片 IRQ2 接口 10b=2d
|  7|  6|  5|  4|  3|    2|    1|    0|
| 0 | 0 | 0 | 0 | 0 | ID2 | ID1 | ID0 |
| 0 | 0 | 0 | 0 | 0 |    0|    1|    0|

从片 B 接了主片 IRQ5 接口 101b=5d
|  7|  6|  5|  4|  3|    2|    1|    0|
| 0 | 0 | 0 | 0 | 0 | ID2 | ID1 | ID0 |
| 0 | 0 | 0 | 0 | 0 |    1|    0|    1|

当中断发生时，主片发送 与从片级联的 IRQ 端口号 所有从片比较 端口号 和自己 ICW3 的低3位就可以了。
因此，从片ICW3 只需要低 3 位

注意:ICW3 需要写入主片的 0x21, 和从片的 0xa1 接口

↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑
*/

/*----------------------------------------
|          ICW4                          |
------------------------------------------
↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓

ICW4:   
|  7|  6|  5|     4|    3|    2|     1|    0|
| 0 | 0 | 0 | SFNM | BUF | M/S | AEOI | μPM |
    - 7~5 位未定义，设置为0
    - SFNM 表示全特殊模式(special fully nested mode) 0 表示全嵌套模式，1 表示特殊全嵌套模式
    - BUF 1 表示工作在缓冲模式,0 为工作在非缓冲模式
    - M/S 当多个8259A 级联时，如果工作在缓冲模式，M/S 为1表示主片，为0 表示从片。如果 非缓冲模式，M/S 无效
    - AEOI 自动结束中断 (auto end of interrupt) 1 为自动中断
    - μPM 微处理器类型， 0 表示 8080 或者 8085 处理器，1 为 x86 处理器

↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑
*/

/*----------------------------------------
|          OCW0                          |
------------------------------------------
↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓
OCW0:
    占位-没有这个字。。
↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑
*/


/*----------------------------------------
|          OCW1                          |
------------------------------------------
↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓

OCW1: 用来屏蔽连接在 8259A 上的外部设备的中断信号
实际上就是把 OCW1 写入了 IMR 寄存器
注意：最终还是受 eflags 寄存器中 IF 位的管束的。即使OCW1 没屏蔽，被 IF 屏蔽了CPU 也不会管

哪一位为1，表示对应IRQ的中断信号被屏蔽
|  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
| M7 | M6 | M5 | M4 | M3 | M2 | M1 | M0 |

↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑
*/



/******************
 * 
 *  ICW1,OCW2,OCW3 通过偶地址端口 0x20 (主片)，0xA0 (从片) 写入
 *  ICW2~ICW4 和 OCW1 用奇地址 0x21 (主片) ，0xA2 (从片) 写入
 * 
 *  4 个 ICW 必须按照顺序写入
 *  由于 OCW 是 ICW 初始化之后进行了，因此 只要往奇地址写数据，就是 OCW1
 * 
 * 
 * 
 * 
******************/


static void pic_init(void)
{

    /*初始化主片*/

    /*
        往主片写ICW1 : 边沿触发(LTIM=0)，级联8259(SNGL=0)
        ，需要 ICW4(IC4=1) (因为需要在ICW4 设置 EOI ，所以需要 ICW4)
        | 7 | 6 | 5 | 4 |     3|    2|     1|    0|
        | 0 | 0 | 0 | 1 | LTIM | ADI | SNGL | IC4 |
        | 0 | 0 | 0 | 1 | 0    | 0   |  0   |   1 |
    */
    outb(PIC_M_CTRL, 0x11);

    /*
        往主片写ICW2 : 起始中断向量号为 0x20,也就是 IR[0-7] 为 0x20~0x27
            0~31 向量号已经被使用。0x20(32) 开始
        |  7 |  6 |  5 |  4 |  3 |   2 |   1 |   0 |
        | T7 | T6 | T5 | T4 | T3 | ID2 | ID1 | ID0 |
        | 0  |  0 |  1 |  0 | 0  |  0  |   0 |   0 |
    */
    outb(PIC_M_DATA, 0x20);

    /*
        往主片写ICW3 : IR2 接从片 100b=0x04
        |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0|
        | S7 | S6 | S5 | S4 | S3 | S2 | S1 | S0|
        |  0 |  0 |  0 |  0 |  0 |  1 |  0 |  0|
    */
    outb(PIC_M_DATA, 0x04);

    /*
        向主片写入ICW4 : 8086 模式（μPM=1），正常 EOI(AEOI=0)(非自动结束中断)
        |  7|  6|  5|     4|    3|    2|     1|    0|
        | 0 | 0 | 0 | SFNM | BUF | M/S | AEOI | μPM |
        | 0 | 0 | 0 |     0|    0|    0|     0|    1|
    */
    outb(PIC_M_DATA, 0x01);     //

    /*初始化从片*/

    /*
        意义和主片类似，端口不同
        ICW1 : 边沿触发，级联8259，需要 ICW4
    */
    outb(PIC_S_CTRL, 0x11);
    /*
        和主片类似。
        ICW2 : 起始中断向量号为 0x28
        也就是 IR[8-15] 为 0x28~0x2F
    */
    outb(PIC_S_DATA, 0x28);

    /*
        ICW3 : 设置从片连接到了主片的 IR2 引脚
        |  7|  6|  5|  4|  3|    2|    1|    0|
        | 0 | 0 | 0 | 0 | 0 | ID2 | ID1 | ID0 |
        | 0 | 0 | 0 | 0 | 0 |    0|    1|    0|    
    */
    outb(PIC_S_DATA, 0x02);
    /*
        与主片类似.
        ICW4 : 8086 模式，正常 EOI
    */
    outb(PIC_S_DATA, 0x01);

    /*
        上面的 ICW 已经初始化完毕，现在对 8259A 发送的任何数据称为 操作控制字，即OCW。
        往 IMR 寄存器发送的命令控制字符为 OCW1 
    */

    /*打开主片上 IR0 也就是目前只接受时钟产生的中断*/
    
    /*
        主片通过 OCW1 设置 只打开 IR0 时钟中断 ，其他关闭
        0xfe=1111_1110b
        |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
        | M7 | M6 | M5 | M4 | M3 | M2 | M1 | M0 |
        |  1 |   1|   1|   1|   1|   1|   1|   0| 
    */
    outb(PIC_M_DATA, 0xfe);
    /*
        从片 IRQ 全部关闭 0xff=1111_1111b
    */
    outb(PIC_S_DATA, 0xff);

    put_str(" pic_init done\n");
}

//function 为在 intr_entry_table 中定义的 中断处理程序的地址
static void make_idt_desc(struct gate_desc *p_gdesc, uint8_t attr, intr_handler function)
{
    p_gdesc->func_offset_low_word = (uin32_t)function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uin32_t)function & 0xFFFF0000) >> 16;
}
//初始化中断描述符表
static void idt_desc_init(void)
{
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++)
    {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    put_str(" idt_desc_init done\n");
}

/*----------------------------------------
|          IDTR                          |
------------------------------------------
↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓
IDTR:
|47                     16|15          0|
|        32位的表基址       | 16位的表界限 |
    0~15 为 IDT 大小减 1 ，表界限
    16~47 IDT 基地址
↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑
*/

//完成有关中断的所有初始化工作
void idt_init()
{
    put_str("idt_init start\n");
    idt_desc_init(); //初始化中断描述符表
    pic_init();      //初始化8259A

    //加载idt
    /*
        低 16 位 是 表界限 idt 大小减 1
        先把 idt 地址类型转换为 uint32_t 然后
    */
    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)((uin32_t)idt << 16)));
    asm volatile("lidt %0" ::"m"(idt_operand));
    put_str("idt_init done\n");
}