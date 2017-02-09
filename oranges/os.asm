
;做 Boot Sector 时一定要讲下面那行注释
;%define _BOOT_DEBUG 
;注释上一行后使用 nasm Boot.asm -o Boot.com 做成一个 .com 文件易于调试

; 调试状态或者 Boot 状态
%ifdef _BOOT_DEBUG  
	  org 0100h
%else 
	org 07c00h 
	;将程序加载到内存偏移07c00h 位置
%endif	

	mov ax,cs
	mov ds,ax
	mov es,ax
	;前面三个指令使得 ds es 两个段寄存器指向与 cs 相同的段
	call DispStr ;调用子程序显示字符串
	jmp $ ; 程序无限循环
;nasm: 不加方括号就是地址，加了就是访问其中的内容	
DispStr:
	mov ax, BootMessage ; 将字符串的首地址赋值给 ax
	mov bp,ax	
	mov cx,16
	mov ax,01301h
	mov bx,000ch
	mov dl,0
	int 10h
	ret
BootMessage:
	db "hello ,OS World!"
; $ 表示当前行被汇编后的地址
; $$ 表示一个 section 开始处的地址 (本程序只有一个section ，实质表示的就是程序编译后的开始地址 也就是 0x7c00)
; $-$$ 表示本行距离程序开始处的相对距离
	times 510-($-$$) db 0  ;重复填充若干个0

	dw 0xaa55 ;如果从软盘启动，检查第一个扇区最后的位置（一个扇区512B)是不是 0xaa55


