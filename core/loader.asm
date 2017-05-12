
%include "boot.inc"

org LOADER_BASE_ADDR


jmp loader_start

LOADER_STACK_TOP equ LOADER_BASE_ADDR 


; gdt 及其内部的描述符
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

;人工对齐:total_mem_bytes4字节+gdt_ptr6字节+ards_buf244字节+ards_nr2,共256字节
ards_buf times 244 db 0

ards_nr dw 0		      ;用于记录ards结构体数量


; ards_nr dw 0		      ;用于记录ards结构体数量



; ards_buf  times  0xb00-($-$$) db 0




loader_start:

;-------  int 15h eax = 0000E820h ,edx = 534D4150h ('SMAP') 获取内存布局  -------

	xor ebx ,ebx 		;第一次调用， ebx 清零 . 之后 每次调用会自动更新 ebx 
	mov edx, 0x534d4150 ;edx 赋值一次。 固定数字
	mov di, ards_buf    ;ards 结构缓冲区， 把所有ards 结构先一次性读取到这里再统一计算
.e820_mem_get_loop:     ; 循环获取每个 ards 内存范围描述结构
	mov eax, 0x0000e820	; 执行 int 0x15 后，  eax 值变为 0x534d4150 ，所以每次循环开始获取ards 时，重新设置 eax 为功能号
	mov ecx, 20         ; ards 地址范围描述符结构 大小为 20 字节
	int 0x15 			; 在进入保护模式之前 先通过 Bios 的 15h 中断 中的  0xe820 方式尝试 获取 内存描述 
	jc .e820_failed_so_try_e801  ; 0xe820 方式获取内存描述失败 ，尝试使用 0xe801 方式
	add di,cx           ; di 增加 20 字节， 使得 di 指向在 ards 缓冲区中的新的索引位置
	inc word [ards_nr]  ; 增加获取到的 ards 描述符结构的 数量 
	cmp ebx,0           ; 如果 ebx 为 0 且 cf 不为 1 表示 ards 全部返回 那就往下执行，计算最大内存大小。否则继续获取下个 ards
	jnz .e820_mem_get_loop

	mov cx ,[ards_nr]	; ards 描述符数量
	mov ebx,ards_buf 	; adrs 描述符缓冲区的首地址
	xor edx,edx  		; edx 保存最大内存容量，先初始化为 0
.find_max_mem_area:
	mov eax, [ebx] 		; base_add_low
	add eax, [ebx+8]    ; length_low 
	add ebx,20          ; 指向缓冲区下一个ards结构
	cmp edx,eax  		; 是否比当前最大的内存大
	jge .next_ards
	mov edx,eax			; 更新最大内存
.next_ards:
	loop .find_max_mem_area ; 如果 cx 不为 0 ,继续遍历，寻找最大内存
	jmp .mem_get_ok 	; 已经获取到最大内存量

; int 15h  ax=e801h 方式获取内存大小，最大支持 4g
; 返回后: ax,cx 值相同，单位为 kb , bx ，dx值相同，单位为 64kb
; ax和 cx 寄存器中为 低 16mb 内存的大小，bx 和 dx 寄存器中为 16mb 到 4g 内存的大小 
.e820_failed_so_try_e801:		
	
	mov ax,0xe801		;功能号
	int 0x15 			
	jc .e801_failed_so_try88

; 1 先计算出低 15mb 的内存 （16mb可能有其他用处)
; ax 和 cx 值相同， 表示低 16 mb 可用内存 。单位为 kb 
	mov cx,0x400 	; 0x400 = 10_0000_0000b = 1kb	
	mul cx			; 16 位乘法  ax *cx => dx ax (dx 放结构的高16位，ax 低16)  
	shl edx,16		; 将 edx 左移动 16 位。那么结果中的dx的16 位就成了最终结果中 应该在的高 16 位 
	and eax,0x0000FFFF ; 将 eax 高 16 位清空
	or edx,eax      ; 合成出 ax * cx 的结果 。也就是 内存数 （单位为 byte) 
	add edx,0x100000; 因为低16mb 中只计算了低15mb ，要加上 16mb中那一个mb  0x100000=1mb 
	mov esi,edx		; 保存低 16 mb 中内存大小到 esi 

	xor eax,eax		; eax 清空
	mov ax,bx 		; bx 保存了 16mb-4gb 内存中内存大小(单位64kb). 放到 ax 作为乘数
	mov ecx,0x10000 ; 0x10000= 1_0000_0000_0000_0000b=64kb 
	mul ecx 		; 32 位乘法。 eax 保存计算结果低32位。
	add esi,eax 	; 与之前的低16mb 内存大小相加
	mov edx,esi 	; 统一用 edx 保存内存大小 
	jmp .mem_get_ok


; int 15h ah=0x88 获取内存大小，只能获取到 64mb 之内
.e801_failed_so_try88:
	

	mov ah,0x88 ; 功能号
	int 0x15 
	jc .error_hlt
	and eax,0x0000FFFF 	; ex 返回 内存大小（kb为单位），这里讲 eax 高16位清空

	mov cx,0x400 	; 乘数 0x400=1kb
	mul cx 			; ax *cx  
	shl edx,16 		; ex 移动到高16位
	or edx,eax 		; 拼接 ax 到 edx 的低16 位 。组成乘法的结果
	and edx ,0x100000; 0x88 只返回 1mb 以上内存。 实际内存要加上 1mb 

.mem_get_ok:

	mov [total_mem_bytes],edx ;保存内存大小

;-----------准备进入保护模式---------------
; 1 打开 a20 地址线
; 2 加载 gdt 
; 3 cr0 的 pe 置 1
	
	; ------- 打开 a20 ----------
	in al ,0x92
	or al, 0000_0010b
	out 0x92,al 

	; -------加载 gdt -----------
	lgdt [gdt_ptr]

	; --------- cr0 第 0 位置为 1 
	mov eax,cr0
	or eax, 0x00000001
	mov cr0,eax

	; 刷新流水线
	jmp SELECTOR_CODE:p_mode_start



.error_hlt:		      ;出错则挂起
    mov sp , LOADER_BASE_ADDR
	mov bp , loader_msg
	mov cx, 17
	mov ax,0x1301
	mov bx,0x001f
	mov dx,0x1800
	int 0x10
	._hlt:
		hlt
		jmp ._hlt


[bits 32]
p_mode_start:
	mov ax, SELECTOR_DATA
	mov ds,ax
	mov es,ax
	mov ss,ax
	mov esp,LOADER_STACK_TOP
	mov ax, SELECTOR_VIDEO
	mov gs,ax 
	mov byte [gs:160], 'p'
	mov byte [gs:2], 0xa4
fin:
	hlt
	jmp fin 	

loader_msg db '2 loader in real.'
