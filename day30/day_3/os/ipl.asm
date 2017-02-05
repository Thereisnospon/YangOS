

; 有关 BOOT_INFO  
CYLS 	equ 	0x0ff0		; 设定启动区
LEDS 	equ 	0x0ff1		
VMODE 	equ 	0x0ff2		; 颜色位数
SCRNX 	equ 	0x0ff4		; 分辨率的 X
SCRNY	equ		0x0ff6		; 分辨率的 Y
VRAM	equ		0x0ff8 		; 图形缓冲区的开始地址

	org 0xc200
	mov	al,0x13
	mov ah,0x00 
	int 0x10 
	mov	byte [VMODE],8
	mov	word [SCRNX],320
	mov	word [SCRNY],200
	mov dword [VRAM],0x000a000

;用 BIOS 取得键盘上各种LED指示灯的状态		
		
	mov ah,0x02 
	int 0x16	;键盘BIOS
	mov [LEDS],AL 
fin:
	hlt
	jmp fin 


