#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "memory.h"
#include "timer.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "tss.h"
#include "syscall-init.h"
#include "ide.h"
#include "fs.h"
void init_all(){
    put_str("init_all\n");
    idt_init();
    mem_init();
    thread_init();
    timer_init();
    console_init();
    keyboard_init();
    tss_init();
    syscall_init(); // 初始化系统调用
    intr_enable();  // 后面的ide_init需要打开中断
    ide_init();     // 初始化硬盘
    filesys_init();
}
