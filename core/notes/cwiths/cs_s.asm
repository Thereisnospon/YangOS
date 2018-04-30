section .data

str:db "asm print",0xa,0
; 0xa 换行。 0 字符串终止
str_len equ $-str 

section .text
;导入外部符号，c 文件中的c函数 : c_print
extern c_print

;导出符号
global _start

_start:
    ;传参，字符串地址
    push str
    ;调用
    call c_print 
    ;清除栈上分配给 字符串地址的参数 的 空间
    add esp,4
    ;调用 1 号系统调用。 exit
    mov eax,1
    int 0x80

global asm_print

asm_print:
    ;保存现场
    push ebp
    mov ebp,esp
    ;4号系统调用。write
    mov eax,4
    ;标准输出 stdout=1
    mov ebx,1
    ;c语言传递的第一个参数
    mov ecx,[ebp+8]
    ;c语言传递的第二个参数
    mov edx,[ebp+12]
    int 0x80
    pop ebp 
    ret 
