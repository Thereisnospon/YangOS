
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

	;创建页目录及页表，并初始化页内存位图
	call setup_page

	; 将描述符表地址及偏移量写入内存 gdt_ptr ，一会用新地址重新加载
	sgdt [gdt_ptr]

	; 将 gdt 描述符中视频段描述符中的段基址 +0xc0000000
	mov ebx, [gdt_ptr+2]
	or dword [ebx+0x18+4],0xc0000000 ;视频段是第3个段描述符,每个描述符是8字节,故0x18。
					      			 ;段描述符的高4字节的最高位是段基址的31~24位
					      			 ; 0xc0000000 =3g 

	; 分页前的 视屏段描述符中的 段基址 在 0xb8000 这是一个物理地址
	; 分页后， 所有的地址是一个线性地址 ，线性地址空间中的 0xb8000 是属于用户空间
	; 显然不能让用户直接操控显存 。于是 将 段基址 加上一个 0xc0000000  这样的话
	; 通过选择子 获得到 段基址后，算出地址 （这个地址现在是线性地址，映射到了内核空间)				      			 


	; 因为之前的 gdt 的地址是没有分页情况下的 地址。 分页后 让它映射到内核中的高地址 
	;将gdt的基址加上0xc0000000使其成为内核所在的高地址
	add dword [gdt_ptr+2],0xc0000000


	add esp ,0xc0000000		; 将栈指针同样映射到内核地址

	; 页目录地址赋值给 cr3
	mov eax,PAGE_DIR_TABLE_POS
	mov cr3,eax 

	; 打开 cr0 的 pg 位 
	mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; 开启分页后，用 gdt 的新的地址从新加载
    lgdt [gdt_ptr]
    mov byte [gs:200],'v'
   
   	mov eax, [esp+2]

    jmp fin 

;------- 创建页目录及页表--------
setup_page:
; 先把页目录占用空间逐个清零	
	mov ecx,4096
	mov esi,0


; PAGE_DIR_TABLE_POS 为 0x100000 = 1mb 
; 页目录放在物理内存 1mb 处

.clear_page_dir:
	mov byte [PAGE_DIR_TABLE_POS+esi],0
	inc esi 
	loop .clear_page_dir

; 创建页目录项 PDE 
.create_pde:
	mov eax, PAGE_DIR_TABLE_POS
	add eax,0x1000 	 ; 0x1000=4kb 此时 eax 为第一个页表的位置 .  页表放在页目录的后面 ,页目录占据 4kb
	mov ebx,eax	     ; 为 creat pte 做准备  ebx 为基址	 	
					 ; 再说下这个 eax  这里指示的是第一个页表的物理地址 应该是 PAGE_DIR_TABLE_POS + eax = 0x100000+0x1000=0x101000 
					 ; 放在 页目录项的 第 0 项 指定的是物理地址 0~0xfffff 的 1mb 的空间 

	or eax, PG_US_U | PG_RW_W | PG_P 
	mov [PAGE_DIR_TABLE_POS+0x0],eax 		; 填入第 0 个页目录项
	mov [PAGE_DIR_TABLE_POS+0xc00],eax		; 填入第 0xc00/4 = 768 项 
											; 这两项都是指向同一个页表 （该页表物理地址为 0x101000  这个页表将来指向的物理地址范围为 0~0xfffff （1mb 空间。其他 3mb 还没分配 )
	
	; 因为加载内核前一直运行的是 Loader 本身在 1mb 内 。因为要保证 之前分段机制中的线性地址和分页后的虚拟地址对应的物理地址一致
	; 第 0 个页表项代表的页表，表示的空间为 0~0x3fffff 包括了这 1mb 内存。 于是用第 0 项保证 loader 在分页下仍然能运行
	; 由于操作系统的虚拟地址在 0xc0000000 （3gb) 以上 高10 位 为 0xc00 	
	; 这里包括了内核的 1mb 物理内存位置 实现了操作系统高 3gb 上的虚拟地址对应到了低端 1mb  									

	sub eax ,0x1000 
	mov [PAGE_DIR_TABLE_POS+4092],eax 		; 最后一个目录项指向页表自己的地址

; 下面创建页表项 PTE 
	mov ecx,256		; 1mb 低端内存  / 页大小 4kb = 256
	mov esi,0 		
	mov edx,PG_US_U | PG_RW_W | PG_P ; 属性为 7 ,US=1,RW=1,P=1

.create_pte:						; 创建 Page table 
	mov [ebx+esi*4],edx 			; 此时的 ebx 赋值为 0x101000 也就是第一个页表的物理地址
	add edx,4096
	inc esi 
	loop .create_pte				; 连续物理地址分配 。第一个页表中与低端 1mb 对应。 

; 创建内核以及其它页表的PDE (这样内核的 3g~4g 的 页目录项就做好了)
	mov eax,PAGE_DIR_TABLE_POS
	add eax,0x2000					; eax 为 第二个页表的位置
	or eax, PG_US_U | PG_RW_W | PG_P
	mov ebx, PAGE_DIR_TABLE_POS		
	mov ecx,254						; 范围为 769~1022 的所有目录项数量
	mov esi,769

.create_kernel_pde:
	mov [ebx+esi*4],eax
	inc esi 
	add eax,0x1000 
	loop .create_kernel_pde
	ret 			



fin:
	hlt
	jmp fin 	

loader_msg db '2 loader in real.'
