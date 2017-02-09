; ==========================================
; pmtest1.asm
; 编译方法：nasm pmtest1.asm -o pmtest1.bin
; ==========================================

%include	"pm.inc"	; 常量, 宏, 以及一些说明

org	0xc200

jmp entry


; 0xc203

fin: jmp		$			; 无限循环


; 0xc205

smsg:
		DB "hello world"
		DB 0x0a 
		DB 0

stack:
	    times 100 db 0

stacktop equ $   

sdata:
		
		times 100 db 1


ok:	db "success"
	db 0


entry:

		; 初始化栈顶
		mov ax ,stacktop  
		mov sp,ax    

	

	

		mov si,smsg
		mov bx,0
		call print


		mov al ,0x70 
		mov bl,4
		mul bl
		mov bx,ax 		; 计算 0x70 号中断在 IVT 中的偏移

		cli  			; 防止改动期间发生新的 0x70 中断

		push es
		mov ax,0x0000
		mov es,ax
		mov word [es:bx],new_int_0x70 ; 偏移地址
		mov word [es:bx+2],cs 		  ; 段地址
		pop es 

		mov al,0x0b 	; RTC 寄存器 B
		or al ,0x80 	; 阻断 NMI
		out 0x70,al 

		mov al,0x12		; 新结束后中断
		out 0x71,al 
		
		mov al,0x0c	
		out 0x70,al 
		mov al,0x71 	; 读 RTC 寄存器 C,复位未决的中断状态

		in al,0xa1		; 清楚 8259 从片的 IMR 寄存器
		and al,0xfe		; 清楚 BIT0 ,此位连接RTC
		out 0xa1,al 	; 写回此寄存器

		sti				; 重新开放中断

		mov si,ok
		mov bx,80+1
		call print

idle:
	hlt 
	not byte [12*160+33*2+1]
	jmp idle



; 正向传送 smsg 的数据到 stack 

movsbtest:

		

		mov ax,smsg
		mov ds,ax
		mov si,0
        
        mov cx,10

        mov ax,sdata
        mov es,ax
        mov di,0

        movsb

        jmp fin



; ----------------------------------|
;-------- 函数:打印字符串-------------|
; 字符串以 0 结束
;  si: 字符串地址
;  bx: 屏幕开始打印处


print:
		push es 
		mov ax,0xb800
		mov es,ax
		jmp ploop
pend:	
		pop es 
		ret 		
ploop:
		mov al,[si]
		cmp al,0
		je pend
		mov ah,0x0c 
		mov [es:bx],ax
		inc si
		add bx,2
		jmp ploop

; ------------------------------ |



new_int_0x70:

		; 保护现场
		push ax
		push bx
		push cx
		push dx
		push es

		; CMOS-RAM 端口
		; 0x70,0x74 是索引端口，指定 CMOS RAM 的内存单元
		; 0x0a 表示寄存器 A 
		; 0x71,0x75 是数据端口，用来读写相应单元的内容
		; 端口0x70 最高位 bit7 是控制 NMI 中断的开关   设置关闭为 0x80 
		; 往 0x70 写 0x80 or 0x0a 表示关闭 NMI ，并且指定 寄存器 A 为内存单元
		; 下面代码是反复测试 BIT7 是否设置了（是否关闭了 NMI)
		w0:
		mov al,0x0a 	; 阻断 NMI 
		or al,0x80 		
		out 0x70,al
		in al,0x71  	; 读寄存器 A 
		test al,0x80    ; 测试第 7位 UIP 
		jnz w0          ; 以上代码对于更新周期结束中断来说是不必要的

		xor al,al
		or al,0x80      ; 秒 为 0x00
		out 0x70,al 	
		in al,0x71 		; 读取 秒
		push ax 		; 暂时保存

		mov al,0x0c 	;  寄存器 C 的索引，且开放 NMI 
		out 0x70,al
		in al,0x71		; 读一下RTG 的寄存器 C 否则只发生一次中断，此处不考虑闹钟和周期性中断的情况

		mov ax,0xb800
		mov es,ax
		pop ax 
		
		call bcd_to_ascii

		mov bx,12*160+36*2
		mov [es:bx],ah
		mov [es:bx+2],al 

		mov al,':'
		mov [es:bx+4],al
		not byte [es:bx+5]

		mov al,0x20 	; 中断结束命令 EOI
		out 0xa0,al 	; 向主片发送
		out 0x20,al 	; 向从片发送

		pop es
		pop dx
		pop cx
		pop bx
		pop ax 

		iret 

bcd_to_ascii:
		mov ah,al
		add al,0x0f
		add al,0x30 
		shr ah,4
		and ah,0x0f
		add ah ,0x30
		ret 
end: xor ax,ax


