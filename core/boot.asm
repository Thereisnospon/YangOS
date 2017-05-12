;----------读取磁盘测试-----------------
; hello-os
; TAB=4

%include "boot.inc"

CYLS	EQU		10				; 定义常量

		ORG		0x7c00			; 程序的装载地址

; 下面是标准FAT12格式软盘专用的代码

		JMP		entry
		DB		0x90
		DB		"HELLOIPL"		; 启动区名称 8 字节
		DW		512				; 每个扇区的大小，必须 512 字节
		DB		1				; 簇(cluster)的大小，必须一个扇区
		DW		1				; FAT 的起始位置，一般第一个扇区开始
		DB		2				; FAT 的个数，必须为 2
		DW		224				; 根目录的大小，一般224项
		DW		2880			; 该磁盘的大小，必须是2880扇区
		DB		0xf0			; 磁盘的种类，必须是 0xf0
		DW		9				; FAT 的长度，必须是 9 扇区
		DW		18				; 一个磁道(track)有几个扇区,必须是18
		DW		2				; 磁头数，必须是 2
		DD		0				; 不使用分区，必须是 0
		DD		2880			; ‚重写一次磁盘大小
		DB		0,0,0x29		; ‚意义不明，固定
		DD		0xffffffff		; (可能是) 卷标号码
		DB		"HELLO-OS   "	; 磁盘名称 11字节
		DB		"FAT12   "		; 磁盘格式名称 8 字节
		RESB	18				; 先空出 18 字节

; 程序核心


entry:
		mov ax,cs 
		mov ds,ax 
		mov es,ax
		mov ss,ax 
		mov fs,ax 
		mov sp,0x7c00
		mov ax,0xb800
		mov gs,ax

		;清屏
		mov ax,0600h
		mov bx,0700h
		mov cx,0
		mov dx,184fh
		int 10h

		;输出字符串 mbr

		mov byte [gs:0x00],'r'
		mov byte [gs:0x01],0xa4



		mov eax,LOADER_START_SECTOR
		mov bx, LOADER_BASE_ADDR
		mov cx,4
		call rd_disk_m_16

;ox7c8b
		jmp 0x900

		; mov byte al , [LOADER_BASE_ADDR]
		; mov byte [gs:0x02],al 
		; mov byte [gs:0x03],0xa4



fin:	
	hlt
	jmp fin 


rd_disk_m_16:
	mov esi,eax
	mov di,cx

	;1
	mov dx,0x1f2
	mov al,cl 
	out dx,al 

	mov eax,esi 

	;2 

	;0~7
	mov dx,0x1f3
	out dx,al 

	;8~15
	mov cl,8
	shr eax,cl 
	mov dx,0x1f4
	out dx,al 

	;16~23
	shr eax,cl 
	mov dx,0x1f5
	out dx,al 

	;24~27
	shr eax,cl 
	and al,0x0f 
	or al ,0xe0  ;lba 
	mov dx,0x1f6 
	out dx,al 

	;3
	mov dx,0x1f7
	mov al,0x20 
	out dx,al 

	not_ready:

	nop 
	in al,dx 
	and al,0x88 
	cmp al,0x08 
	jnz not_ready

	mov ax,di 
	mov dx,256
	mul dx 
	mov cx,ax 

	mov dx,0x1f0 

	go_on_read:
	in ax,dx 
	mov [bx],ax 
	add bx ,2 
	loop go_on_read
	ret 

	times 510-($-$$) db 0
	db 0x55,0xaa 
