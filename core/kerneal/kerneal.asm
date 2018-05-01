[bits 32]
%define ERROR_CODE nop  ;若在相关的异常中cpu已经自动压入了错误码
                        ;为了保持栈中格式统一，这里不做操作
%define ZERO push 0     ;若在相关的异常中，cpu没有压入错误码
                        ;为了保持栈中格式统一，手工压入个0

extern put_str ; 声明外部函数

section .data 
intr_str db "interrupt occur!", 0xa,0
global intr_entry_table

intr_entry_table:
; macro
%macro VECTOR 2
section .text 
intr%1entry: ;每个中断处理程序唯一标记
    %2              ;压入错误码
    push intr_str   ;打印字符串
    call put_str
    add esp,4       ;清除参数

    ;如果是从片上进入的中断，除了往从片发送 EOI ，还得往 主片发送 EOI
    mov al,0x20     ;中断结束命令 EOI
    out 0xa0,al     ;向从片发送
    out 0x20,al     ;向主片发送

    add esp,4       ;跨过 ERROR_CODE    
    iret            ; 从中断返回，32 位下等同于 iretd
section .data       
    dd intr%1entry  ;存储各个中断处理程序的地址
                    ;形成一个 inter_entry_table 数组
%endmacro
; end macro


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