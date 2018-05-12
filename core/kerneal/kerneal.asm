[bits 32]
%define ERROR_CODE nop  ;若在相关的异常中cpu已经自动压入了错误码
                        ;为了保持栈中格式统一，这里不做操作
%define ZERO push 0     ;若在相关的异常中，cpu没有压入错误码
                        ;为了保持栈中格式统一，手工压入个0

extern put_str ; 声明外部函数
extern idt_table ; c 中中断处理程序数组

section .data 
global intr_entry_table

intr_entry_table:
; macro
%macro VECTOR 2
section .text 
intr%1entry: ;每个中断处理程序唯一标记
    %2              ;压入错误码



    ;保存环境
    push ds
    push es 
    push fs 
    push gs 
    pushad ;32 位寄存器。 入栈顺序
           ; eax ecx edx ebx esp ebp esi edi 

    ;如果是从片上进入的中断，除了往从片发送 EOI ，还得往 主片发送 EOI
    mov al,0x20     ;中断结束命令 EOI
    out 0xa0,al     ;向从片发送
    out 0x20,al     ;向主片发送

    push %1         ;不管 idt_table 目标程序是否需要参数，一律压入中断向量号,为了调试
  
    call [idt_table+ %1*4] ; idt_table 中c版本的中断处理函数
                           ; idt_table 每个元素是 32位地址，4字节。
    jmp intr_exit

section .data       
    dd intr%1entry  ;存储各个中断处理程序的地址
                    ;形成一个 inter_entry_table 数组
%endmacro
; end macro

section .text 
global intr_exit
intr_exit:
;以下恢复上下文环境
    add esp,4   ;跳过中断号
    popad
    pop gs 
    pop fs 
    pop es 
    pop ds 
    add esp,4   ;跳过 error code
    iretd 


VECTOR 0x00,ZERO
VECTOR 0x01,ZERO
VECTOR 0x02,ZERO
VECTOR 0x03,ZERO 
VECTOR 0x04,ZERO
VECTOR 0x05,ZERO
VECTOR 0x06,ZERO
VECTOR 0x07,ZERO 
VECTOR 0x08,ERROR_CODE
VECTOR 0x09,ZERO
VECTOR 0x0a,ERROR_CODE
VECTOR 0x0b,ERROR_CODE 
VECTOR 0x0c,ZERO
VECTOR 0x0d,ERROR_CODE
VECTOR 0x0e,ERROR_CODE
VECTOR 0x0f,ZERO 
VECTOR 0x10,ZERO
VECTOR 0x11,ERROR_CODE
VECTOR 0x12,ZERO
VECTOR 0x13,ZERO 
VECTOR 0x14,ZERO
VECTOR 0x15,ZERO
VECTOR 0x16,ZERO
VECTOR 0x17,ZERO 
VECTOR 0x18,ERROR_CODE
VECTOR 0x19,ZERO
VECTOR 0x1a,ERROR_CODE
VECTOR 0x1b,ERROR_CODE 
VECTOR 0x1c,ZERO
VECTOR 0x1d,ERROR_CODE
VECTOR 0x1e,ERROR_CODE
VECTOR 0x1f,ZERO 
VECTOR 0x20,ZERO


global get_eip
get_eip:
    mov eax,[esp]
    ret

; // ;          --           --
; // ;          | 0 | ss_old  |  <-----  发送特权级切换时 压入的旧栈段 ss   (cpu压入)
; // ;          --           --
; // ;          |   esp_old   |  <-----  发送特权级切换时 压入的旧栈指针 esp (cpu压入)
; // ;          --           --
; // ;          |  EFLAGS     |  <----- 标识位寄存器 (cpu压入)
; // ;          --           --
; // ;          |  0 | cs_old |  <----- (cpu压入)
; // ;          --           -- 
; // ;          |   eip_old   |  <----- (cpu压入)
; // ;          --           --  <-------- esp (new) step0: 进入中断处理程序
; // ;          |  error_code |  <-------- step1: 压入 error_code 或者占位的 0 
; // ;          --           -- 
; // ;          |    ds       | 
; // ;          --           --
; // ;          |    es       | 
; // ;          --           --
; // ;          |    fs       | 
; // ;          --           --
; // ;          |    gs       | 
; // ;          --           -- ←←←←←←←←←←↑
; // ;          |    eax      |           ↑
; // ;          --           --           ↑
; // ;          |    ecx      |           ↑
; // ;          --           --           ↑
; // ;          |    edx      |           ↑
; // ;          --           --           ↑
; // ;          |    ebx      |           ↑
; // ;          --           --           ↑
; // ;          |    esp      | →→→→→→→→→→↑ 压入 pushad 时，esp 应该指向的是 gs 的位置
; // ;          --           --
; // ;          |    ebp      | 
; // ;          --           --
; // ;          |    esi      | 
; // ;          --           --
; // ;          |    edi      | <------ step2: 压入 ds,es,fs,gs pushad 寄存器
; // ;          --           --
; // ;          |  intr_no    | <----- 压入 中断处理函数的参数 （中断向量号
; // ;          --           --
; // ;          |  ret_addr   | <----- 调用 中断处理函数，压入返回地址
; // ;          --           --
; // ;          |             | 
; // ;          --           --




; // ;          --           --
; // ;          | 0 | ss_old  |  <-----  发送特权级切换时 压入的旧栈段 ss   (cpu压入)
; // ;          --           --
; // ;          |   esp_old   |  <-----  发送特权级切换时 压入的旧栈指针 esp (cpu压入)
; // ;          --           --
; // ;          |  EFLAGS     |  <----- 标识位寄存器 (cpu压入)
; // ;          --           --
; // ;          |  0 | cs_old |  <----- (cpu压入)
; // ;          --           -- 
; // ;          |   eip_old   |  <----- (cpu压入) step5: iretd 弹出 eip_old,eip_old 等
; // ;          --           --  
; // ;          |  error_code |  step4: add esp,4 跳过 error_code 
; // ;          --           -- 
; // ;          |    ds       | pop ds
; // ;          --           --
; // ;          |    es       | pop es
; // ;          --           --
; // ;          |    fs       | pop fs
; // ;          --           --
; // ;          |    gs       | pop gs step3： pop gs,fs,es,ds 等
; // ;          --           -- ←←←←←←←←←←↑ <------ step2: popad 时，esp 重新指向 gs
; // ;          |    eax      |           ↑
; // ;          --           --           ↑
; // ;          |    ecx      |           ↑
; // ;          --           --           ↑
; // ;          |    edx      |           ↑
; // ;          --           --           ↑
; // ;          |    ebx      |           ↑
; // ;          --           --           ↑
; // ;          |    esp      | →→→→→→→→→→↑ 压入 pushad 时，esp 应该指向的是 gs 的位置
; // ;          --           --
; // ;          |    ebp      | 
; // ;          --           --
; // ;          |    esi      | 
; // ;          --           --
; // ;          |    edi      |  
; // ;          --           --
; // ;          |  intr_no    | <------- step1 : add esp,4 跳过中断向量号
; // ;          --           --
; // ;          |  ret_addr   | <------- step0 : ret 返回，弹出 ret_addr 并跳转
; // ;          --           --
; // ;          |             | 
; // ;          --           --