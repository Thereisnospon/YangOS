
%include "boot.inc"

org LOADER_BASE_ADDR


jmp loader_start

LOADER_STACK_TOP equ LOADER_BASE_ADDR 


GDT_BASE dd 0x00000000 
	     dd 0x00000000

CODE_DESC: dd 0x0000FFFF      ; 低 32 位 
		   dd DESC_CODE_HIGH4 ; 高 32 位 
		  ; 段基址为 0 
		  ; 段界限为  0xFFFFF G 位为 1  0xFFFFF * 4k =4g 

; 数据段和代码段用同一个段 
DATA_STACK_DESC: dd 0x0000FFFF
 				 dd DESC_DATA_HIGH4


; 拼装后的 段基址为 0xb8000  ， 
VIDEO_DESC: dd 0x80000007	 ; limit= (0xbffff-0xb000) / 4k = 0x7
		    dd DESC_VIDEO_HIGH4 

GDT_SIZE equ $-GDT_BASE
GDT_LIMIT equ GDT_SIZE-1

; 预留 60 个描述符位
times 60 dq 0 

SELECTOR_CODE equ  (0x0001<<3) + TI_GDT + RPL0
SELECTOR_DATA equ  (0x0002<<3) + TI_GDT + RPL0
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0

; total_mem_bytes用于保存内存容量,以字节为单位,此位置比较好记。
; 当前偏移loader.bin文件头0x200字节,loader.bin的加载地址是0x900,
; 故total_mem_bytes内存中的地址是0xb00.将来在内核中咱们会引用此地址
total_mem_bytes dd 0 

;gdt 指针
gdt_ptr dw GDT_LIMIT  ; gdt 界限
		dd GDT_BASE   ; gdt 起始地址


loader_msg db '2 loader in real.'

loader_start:

	mov sp , LOADER_BASE_ADDR
	mov bp , loader_msg
	mov cx, 17
	mov ax,0x1301
	mov bx,0x001f
	mov dx,0x1800
	int 0x10

	in al ,0x92
	or al, 0000_0010b
	out 0x92,al 

	lgdt [gdt_ptr]

	mov eax,cr0
	or eax, 0x00000001
	mov cr0,eax

	jmp SELECTOR_CODE:p_mode_start

[bits 32]
p_mode_start:
	mov ax, SELECTOR_DATA
	mov ds,ax
	mov es,ax
	mov ss,ax
	mov esp,LOADER_STACK_TOP
	mov ax, SELECTOR_VIDEO
	mov gs,ax 
	mov byte [gs:1], 'p'
	mov byte [gs:2], 0xa4
fin:
	hlt
	jmp fin 	


