#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"
#include "print.h"
#include "debug.h"

#define IDT_DESC_CNT 0x81 //目前支持的中断数

#define PIC_M_CTRL 0x20 //主片控制端口 0x20
#define PIC_M_DATA 0x21 //主片数据端口 0x21
#define PIC_S_CTRL 0xa0 //从片控制端口 0xa0
#define PIC_S_DATA 0xa1 //从片数据端口 0xa1

#define EFLAGS_IF 0x00000200 //eflags 寄存器 if 位为1
/*
    eflags push 到了栈中，然后 pop 到 g(寄存器或者内存中) 
*/
#define GET_EFLAGS(EFLAG_VAR) asm volatile ("pushfl; popl %0":"=g"(EFLAG_VAR))

extern uint32_t syscall_handler(void);

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

char* intr_name[IDT_DESC_CNT];          //保存异常的名字
intr_handler idt_table[IDT_DESC_CNT];   //

// intr_handler 是指针类型 ， intr_entry_table 是需要引入的符号名 且该符号是个数组,中断处理程序的入口
extern intr_handler intr_entry_table[IDT_DESC_CNT]; //声明定义在 kerneal.asm 中的中断处理函数入口数组

/*通用的中断处理函数，一般在异常出现时处理*/
static void general_intr_handler(uint8_t vec_nr)
{
    if (vec_nr == 0x27 || vec_nr == 0x2f)
    {
        // IRQ7 和 IRQ15 会产生伪中断 (spurious interrupt) 无需处理
        //0x2f 是从片最后一个引脚，保留项
        return;
    }
    print_debugln("<interrupt>");
    print_debug_kv_str("intr_name",intr_name[vec_nr]);
    print_debug_kv_int("vec_no",vec_nr);
    if (vec_nr == 14)
    {
        int page_fault_addr = 0;
        //cr2 存放 page_falut 地址
        asm("movl %%cr2,%0"
            : "=r"(page_fault_addr));
        print_debug_kv_int("page_fault_addr",page_fault_addr);
    }
    print_debugln("</interrupt>");
    //能进入中断处理程序，说明已经处在关中断情况下，下面死循环不会被中断
    while (1)
    {
        ;
    }
}
//一般中断处理函数名注册和异常名称注册
static void exception_init(void){
    int i;
    for(i=0;i<IDT_DESC_CNT;i++){
        // idt_table 数组中的函数是在进入中断后根据中断向量号调用的
        // 见 kerneal/kerneal.asm 的 call [idt_table+%1*4]
        idt_table[i] = general_intr_handler; // 默认处理函数为 general_intr_handler
        intr_name[i]="unknow";  //统一赋值名字 unknow
    }
    intr_name[0]="#DE Divide Error";
    intr_name[1]="#DB Debug Exception";
    intr_name[2]="NMI Interrupt";
    intr_name[3]="#BP Breakpoint Exception";
    intr_name[4]="#OF Overflow Exception";
    intr_name[5]="#BR Bound Range Exceeded Exception";
    intr_name[6]="#UD Invalid Opcode Exception";
    intr_name[7]="#NM Device Not Available Exception";
    intr_name[8]="#DF Double Fault Exception";
    intr_name[9]="Coprocessor Segment Overrun";
    intr_name[10]="#TS Invalid TSS Exception";
    intr_name[11]="#NP Segment Not Present";
    intr_name[12]="#SS Stack Fault Exception";
    intr_name[13]="#GP General Protection Exception";
    intr_name[14]="#PF Page-Fault Exception";
    //15 是 intel 保留项 未使用
    intr_name[16]="#MF x87 FPU Floating-Point Error";
    intr_name[17]="#AC Aligment Check Exception";
    intr_name[18]="#MC Machine-Check Exception";
    intr_name[19]="#XF SIMD Floating-Point Exception";
}

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
    //outb(PIC_M_DATA, 0xfe);
    /*
        从片 IRQ 全部关闭 0xff=1111_1111b
    */
    //outb(PIC_S_DATA, 0xff);

    //IRQ2 打开（级联从片), 主片的IRQ0 时钟中断， IRQ1 的键盘中断
    // 1111 1000 =0xf8
    outb(PIC_M_DATA,0xf8);
    //打开从片的 IRQ14,硬盘控制器
    // 1011 1111 =0xbf
    outb(PIC_S_DATA,0xbf);
    put_str(" pic_init done\n");
}

//function 为在 intr_entry_table 中定义的 中断处理程序的地址
static void make_idt_desc(struct gate_desc *p_gdesc, uint8_t attr, intr_handler function)
{
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}
//初始化中断描述符表
static void idt_desc_init(void)
{
    int i,lastindex=IDT_DESC_CNT-1;
    for (i = 0; i < IDT_DESC_CNT; i++)
    {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    //0x80 系统调用对应的中断门的 dep为 3
    make_idt_desc(&idt[lastindex], IDT_DESC_ATTR_DPL3, syscall_handler);
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
    exception_init(); //异常名初始化，并注册通常的中断处理函数
    pic_init();      //初始化8259A

    //加载idt
    /*
        低 16 位 是 表界限 idt 大小减 1
        先把 idt 地址类型转换为 uint32_t 然后
    */
    /*
    一个巨坑，查了 一整天的问题 多了一个括号。。。。
        貌似这样写先强转 32 位然后左移 16 位 再强转64. 可能会丢失精度。（下面正确的是 转换成 64 位再左移16位
    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16)));
    问题查询（崩溃前的info tab 查看中断表）：
        Interrupt Descriptor Table (base=0x00000000c00071a0, limit=383): （正确形式的左移
        Interrupt Descriptor Table (base=0x00000000000071a0, limit=383):  (错误的左移
    可以看出 32 位数据的 0xc00071a0 左移 16 位 会正好丢掉 高位的 c0 部分 成为 0x71a0 ,然后 转换成 64位
    而正确形式的转换是 转换成了 64 位再左移 ，高位没有丢弃， 仍然是 0xc00071a0
    再看崩溃的地方，
    enum intr_status intr_enable() {
        enum intr_status old_status;
        if(INTR_ON==intr_get_status()){
            old_status=INTR_ON;
            return old_status;
        }else{
            old_status=INTR_OFF;
            asm volatile("sti"); //开中断 sti 指令把 IF 位置 1
            return old_status; // 汇编指令为 :
                               //  leave 每次都是断点在这里再 s 继续执行的时候崩溃了
                               //  ret
        }
    }
    每次都是在 leave 执行的时候崩溃，leave 相当于: mov esp ebp ; pop ebp . 
    一开始查的是页表的问题，（禁止切换经常页表时程序正常执行） 怀疑 esp,ebp 有问题。
    根据以上分析，应当是这样的:
        
        1. 切换进程前，激活了进程的页表。 进程的页表只拷贝了内核的 0x300 项后面的部分
          
        cr3: 0x0000000000100000 （激活进程页表前的页表）
        0x00000000-0x000fffff -> 0x0000000000000000-0x00000000000fffff （映射了虚拟地址 0x0~0xfffff 到物理地址 0x0~0xfffff
        0x00100000-0x00118fff -> 0x0000000000200000-0x0000000000218fff
        0xc0000000-0xc00fffff -> 0x0000000000000000-0x00000000000fffff  (映射了虚拟地址 0xc0000000~0xc00fffff 到物理地址 0x0~0xfffff
        0xc0100000-0xc0118fff -> 0x0000000000200000-0x0000000000218fff
        0xffc00000-0xffc00fff -> 0x0000000000101000-0x0000000000101fff
        0xfff00000-0xffffefff -> 0x0000000000101000-0x00000000001fffff
        0xfffff000-0xffffffff -> 0x0000000000100000-0x0000000000100fff

        cr3: 0x0000000000218000
        0xc0000000-0xc00fffff -> 0x0000000000000000-0x00000000000fffff 只映射了虚拟地址 0xc0000000~0xc00fffff 到物理地址 0x0~0xfffff
        0xc0100000-0xc0118fff -> 0x0000000000200000-0x0000000000218fff
        0xfff00000-0xffffefff -> 0x0000000000101000-0x00000000001fffff
        0xfffff000-0xffffffff -> 0x0000000000218000-0x0000000000218fff

        看出，激活进程页表后，缺少了 低地址的映射，而此时的 idt 地址为 0x71a0

        2. leave 指令前 执行了 开中断
        3. 预测应该是 开中断时 正好出现了一个中断，然后 cpu 查找 idtr 寄存器查找到 0x71a0
        4. 由于开启了分页，所有地址视为虚拟地址，然后去进程的 页表查，没有查到该 地址对应的页表信息 ？ 然后应该又引发缺页中断 
           去查找缺页中断处理程序，又去查找 idtr 。然后就崩溃了。。。
        看上去是在 leave 指令上崩溃的，其实是正好在这里响应中断崩了。。

        问题就是 计算 idt 地址的时候 损失了高位。。 但是正好 0x71a0 这个地址对于 之前的内核是进行了映射的，所以处理正常
        而用户进程没有这段的页表。。
    */
    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)(uint32_t)idt << 16));
    asm volatile("lidt %0" ::"m"(idt_operand));
    put_str("idt_init done\n");
}
//开中断，返回开中断前的状态
enum intr_status intr_enable(){
    enum intr_status old_status;
    if(INTR_ON==intr_get_status()){
        old_status=INTR_ON;
        return old_status;
    }else{
        old_status=INTR_OFF;
        asm volatile("sti"); //开中断 sti 指令把 IF 位置 1
        return old_status;
    }
}
//关中断，返回关中断前的状态
enum intr_status intr_disable(){
    enum intr_status old_status;
    if(INTR_ON==intr_get_status()){
        old_status=INTR_ON;
        asm volatile("cli":::"memory");//关中断 cli 指令把 IF 位置0
        return old_status;
    }else{
        old_status=INTR_OFF;
        return old_status;
    }
}
//设置中断状态
enum intr_status intr_set_status(enum intr_status status){
    return status & INTR_ON ? intr_enable() : intr_disable();
}
//获取中断状态
enum intr_status intr_get_status(){
    uint32_t eflags=0;
    GET_EFLAGS(eflags);
    return (EFLAGS_IF & eflags) ? INTR_ON : INTR_OFF;
}
//安装中断处理程序
void register_handler(uint8_t vector_no, intr_handler function)
{
    idt_table[vector_no] = function;
}