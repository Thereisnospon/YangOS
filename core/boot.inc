
; loader 在硬盘的起始扇区号
LOADER_START_SECTOR equ 0x2
LOADER_BASE_ADDR equ 0x900
LOADER_STACK_TOP equ LOADER_BASE_ADDR
LOADER_CURRENT_SECTOR_SIZE equ 4

; kerneal.bin 加载到内存的 buffer 区域
KERNEL_BIN_BASE_ADDR equ 0x70000
KERNEL_IMAGE_BASE_ADDR equ 0x1500
; kerneal.bin 在硬盘的扇区号
KERNEL_START_SECTOR equ 0x9 

KERNEL_ENTRY_POINT equ 0xc0001500

; 页目录位置
PAGE_DIR_TABLE_POS equ 0x100000

CONSTANT_MEM_3G equ 0xc0000000

KERNEL_STACK_START equ 0xc009f000

; gdt 高 32 位																					     gdt 低 32 位

; | 段基址 31~24  | 23 | 22 | 21 | 20  | 19~16      | 15 | 14~13 | 12 | 11~8 | 7~0        |  |  31~16	            | 15~0              |
; | 00000000     | G  | D/B| L  | AVL | 段界限 19~16| P  | DPL   | S  | TYPE | 段基址 23~16|  | 段基址 15~0        | 段界限 15~0        |
; | 00000000     | 0  | 0  | 0  | 0   | 0000       | 0  | 00    | 0  | 0000 | 00000000   |  | 00000000_00000000 | 00000000_00000000 |		 


;--------------   gdt描述符属性  -------------
DESC_G_4K   equ	  1_00000000000000000000000b   
DESC_D_32   equ	   1_0000000000000000000000b
DESC_L	    equ	    0_000000000000000000000b	;  64位代码标记，此处标记为0便可。
DESC_AVL    equ	     0_00000000000000000000b	;  cpu不用此位，暂置为0  
DESC_LIMIT_CODE2  equ 1111_0000000000000000b	; 
DESC_LIMIT_DATA2  equ DESC_LIMIT_CODE2
DESC_LIMIT_VIDEO2  equ 0000_000000000000000b
DESC_P	    equ		  1_000000000000000b        ; 存在位
DESC_DPL_0  equ		   00_0000000000000b        ; 特权级 0
DESC_DPL_1  equ		   01_0000000000000b        ; 特权级 1 
DESC_DPL_2  equ		   10_0000000000000b        ; 特权级 2
DESC_DPL_3  equ		   11_0000000000000b        ; 特权级 3
DESC_S_CODE equ		     1_000000000000b        ; 数据段 
DESC_S_DATA equ	  DESC_S_CODE                   ; 数据段 
DESC_S_sys  equ		     0_000000000000b        ; 系统段
DESC_TYPE_CODE  equ	      1000_00000000b	    ;x=1,c=0,r=0,a=0 代码段是可执行的,非依从的,不可读的,已访问位a清0.  
DESC_TYPE_DATA  equ	      0010_00000000b	    ;x=0,e=0,w=1,a=0 数据段是不可执行的,向上扩展的,可写的,已访问位a清0.



DESC_CODE_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + DESC_LIMIT_CODE2 + DESC_P + DESC_DPL_0 + DESC_S_CODE + DESC_TYPE_CODE + 0x00
DESC_DATA_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + DESC_LIMIT_DATA2 + DESC_P + DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x00
DESC_VIDEO_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + DESC_LIMIT_VIDEO2 + DESC_P + DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x0b

;--------------   选择子属性  ---------------
RPL0  equ   00b
RPL1  equ   01b
RPL2  equ   10b
RPL3  equ   11b
TI_GDT	 equ   000b
TI_LDT	 equ   100b


; PDE 页表目录项
; | 31~12         ：   | 11~9  | 8 | 7   | 6 | 5 | 4   | 3   | 2  | 1  | 0 |
; | 页表物理页地址 31~12 | AVL   | G | 0   | D | A | PCD | PWT | US | RW | P |
; PTE 页表项
; | 31~12         ：   | 11~9  | 8 | 7   | 6 | 5 | 4   | 3   | 2  | 1  | 0 |
; | 物理页地址 31~12 ：：| AVL   | G | PAT | D | A | PCD | PWT | US | RW | P |

;  -P (Present) 存在位 1 表示在物理内存中，0不在
;  -RW (Read/Write) 读写位 1 可读可写 0 可读不可写
;  -US (User/Supervisor) 普通用户/超级用户位 1 为 User 级别 任意特权级(0,1,2,3)可访问
;     0 为 Supervisor 级别，特权级3不可访问，(0,1,2 可以)
; -PWT (Page-Level Write-Throught) 页级通写位 1 表示用通写方式，不仅是普通内存，还是高速缓存
; -PCD (Page-Level Cache-Disable) 页级高速缓存禁止位，1 启用高速缓存，0 禁用
; -A (Accesed) 访问位 1 表示cpu访问过
; -D (Dirty) 脏页位 cpu对一个页执行写操作时，将它置为1，此项只对页表项有效，不会修改页目录项
; -PAT (Page Attribute Table) 页属性表位，能够在页面一级的粒度上设置内存属性
; -G (Gloabl) 全局位， 1 为全局页，一直在高速缓存TLB 保存
; -AVL (Available) 位，表示是否可用，操作系统可以用该位





;----------------   页表相关属性    --------------
PG_P  equ   1b
PG_RW_R	 equ  00b 
PG_RW_W	 equ  10b 
PG_US_S	 equ  000b 
PG_US_U	 equ  100b 

PT_NULL equ 0


;     前景色       背景色
; |15|14|13|12|11|10|9|8|7~0   |
; |K |R | G| B|I |R |G|B| ASCII|
; k:闪烁 i:亮度



; <--+-------------+-->
;    |             |
;    |             |
;    |             |
; <------------------->
;    |             |
;    |             |
;    |             |
; <------------------->
;    |             |
;    |             |
;    |             |
; <------------------->
;    |             |
;    |             |
;    |             |
; <--+-------------+-->





;******************低端 1MB 空间****************************
;----------------------------------------------------------
; <--+-------------+--> 0x9FFFF
;    |             |  EBDA (Extend BIOS Data Area)  
;    |     1K      |  扩展 BIOS 数据区
;    |             |
; <-------------------> 0x9FC00
; <-------------------> 0x9FBFF
;    |    +++      |    
;    |   约608K    |  可用区域 (kerneal.bin)
;    |             |
; <-------------------> 0x7E00
; <-------------------> 0x7DFF
;    |    +++      |
;    |   512B      | MBR 被 BIOS 加载到此区域
;    |             |
; <-------------------> 0x7c00
; <-------------------> 0x7DFF
;    |    +++      |
;    |   约30K     |  可用区域 （loader.bin, gdt
;    |             |
; <--+-------------+--> 0x500
; <-------------------> 0x4ff
;    |             | BIOS Data Area
;    |    256B     | BIOS 数据区
;    |             |
; <--+-------------+--> 0x400
; <-------------------> 0x3ff
;    |             |
;    |    1K       | Interrupt Vector Table 中断向量表
;    |             |
; <--+-------------+--> 0x000
;**********************************************************
;----------------------------------------------------------


; <--+-------------+--> 0x9f000 esp 内核栈底
;    |             |  
;    |             |  0x9f000 ~ 0x1500 约 630KB 栈空间，通常不会碰撞内核。
;    |             |  下面的区域可以被覆盖
; <-------------------> 0x9FC00
; <-------------------> 0x9FBFF
;    |             |    
;    |   190KB     |  kerneal.bin
;    |             |
; <-------------------> 0x70000
; <-------------------> 
;    |             |
;    |             |
;    |             |
; <--+-------------+--> 0x7E00
; <-------------------> 0x7DFF
;    |             |
;    |             |  内核
;    |             |
; <--+-------------+--> 0x1500 内核映像初始地址。 与 0x1500,0xc0001500 虚拟地址映射
; <-------------------> 0x14FF
;    |             |
;    |             |    loader.bin (文件开始是gdt，不移动位置)
;    |             |
; <--+-------------+--> 0x900
; <-------------------> 
;    |             |
;    |             |    可用空间
;    |             |
; <--+-------------+--> 0x500





; <-------------------> 
;    |             |
;    |             |
;    |             |
; <--+-------------+--> 