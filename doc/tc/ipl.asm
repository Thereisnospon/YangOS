; ==========================================
; pmtest1.asm
; 编译方法：nasm pmtest1.asm -o pmtest1.bin
; ==========================================

%include	"pm.inc"	; 常量, 宏, 以及一些说明

org	0xc200



jmp entry


stack:
	    times 256 db 0

stacktop equ $  	


smsg:
		DB "str"	
		db 0
		smsglen equ $-smsg



s1:
	db "ab"
	db 0
s2: 
	db 'abc'
	db 0	

entry:


		; 初始化栈顶
		mov ax ,stacktop  
		mov sp,ax
		sub sp,10
		mov bp,ax


		mov si ,sp 
		mov word [si],1
		call absNum

		
		mov si ,sp 
		mov word [si],ax 
		call showNum


		; mov si ,sp 
		; mov word [si],s1 
		; mov word [si+2],s2 
		; call strcmp

		
		; mov si,sp 
		; mov [si],ax 
		; call showNum


		jmp fin


fin:
		hlt					; CPU 停止，等待指令
		jmp		fin				; 无限循环




; 样例 1 ， 两面分别打印字符串， 并显示第二面
sample1:

		mov si,sp 
		mov word [si],smsg   ;    第一个参数 
		mov word [si+2],smsglen	; 第二个参数
		call showmsg
		
		mov si,sp 
		mov word [si],smsg   ;    第一个参数 
		mov word [si+2],smsglen	; 第二个参数
		call showmsg3				;
		
		;显示第二页
		mov si,sp
		mov byte [si], 0x01
		call setPage
		jmp fin 



; 设置显示页数 setPage
; 参数
; page byte 页数
; 用例
; mov si,sp
; mov byte [si], 0x01	; 第 0x01 页
; call setPage

setPage:
	push bp 
	mov bp,sp 
	mov ah ,0x05	;功能号
	mov byte al ,[bp+4]	;显示页数
	int 10h
	leave
	ret


; 设置光标位置
; 参数
; page byte 页数
; row  byte 行数
; col  byte 列数
; 用例:
; mov si,sp 
; mov byte [si] , 0	 ; 第 0 页 
; mov byte [si+2],10 ; 第 10 行 
; mov byte [si+4],5	 ; 第 5 列 
; call  setYX

setYX:
	push bp 
	mov bp,sp 
	mov ah ,0x02  ; 功能号
	mov byte bh ,[bp+4]	  ; 页码
	mov byte dh ,[bp+6]	  ; 行号  	
	mov byte dl ,[bp+8]	  ; 列号
	int 10h
	leave
	ret




; 设置显示器模式为 80×25 16色 文本
setMode1:
	push bp 
	mov bp,sp 
	mov ax,0x00  ; 设置显示器模式 功能号
	mov al,0x02  ; 02H：80×25 16色 文本
	int 10h
	leave
	ret

; 设置显示器模式 为 320×200 16色
setMode2:
	push bp 
	mov  bp,sp
	mov ax ,0x00 ; 设置显示模式 功能号
	mov al, 0x0d ; 0x0d 表示 320×200 16色
	int 10h 
	leave
	ret 


; 输出一个像素
; 显示器必须为图形模式
drawP:
 	push bp 
 	mov bp ,sp 
 	mov ah ,0x0c ;功能号
 	mov al ,0xff  ;像素值
 	mov bh ,0	;页码
 	mov cx ,10	;X
 	mov dx ,10	;Y
 	int 10h
 	leave
 	ret

; 绝对值
absNum:
	push bp 
	mov bp,sp 
	mov ax,[bp+4]
	cmp ax,0
	jns abs_end
	not ax
	inc ax
abs_end:
	leave
	ret


;指定页的当前光标显示字符
;参数
; char byte 字符
; page byte 页数
; 用例 
; mov si,sp 
; mov byte [si],'w'
; mov byte [si+2],0
; call drawCharInPage
drawCharInPage:
	push bp 
	mov bp ,sp 	
	mov ah, 0x09 ;功能号
	mov al, [bx+4]	 ;字符 
	mov bh, [bx+6]	 ;页号
	mov bl, 0fh  ;属性(文本模式)或颜色(图形模式)
	mov cx,1	 ;重复次数
	int 10h
	leave 
	ret 

; 在第 0  页显示字符
; 参数
; char byte 字符 
; 用例
; mov si,sp 
; mov byte [si],'w'
; call drawChar
drawChar:
	push bp 
	mov bp ,sp 	
	mov ah, 0x09 ;功能号
	mov al, [bp+4]	 ;字符 
	mov bl, 0fh  ;属性(文本模式)或颜色(图形模式)
	mov cx,1	 ;重复次数
	int 10h
	leave 
	ret 







; 在当前光标显示数字
; 参数
; num  word 数字 : [bp+4]
showNum:
	push bp 
	mov  bp,sp
	push si 
	push bx
	push di 
	xor si ,si  
	mov  di ,sp 
	mov  ax,[bp+4]	; 被除数
	mov bx,ax
	shl bx ,1
	jnc	strnum_go ; 如果是负数
	mov si ,1
	not ax
	inc ax  
	strnum_go:
	cmp  ax,0	; 判断被除数
	je   show_num_0_ ; 如果被除数为0，直接输出0
	mov  bx ,10
	show_num_loop:
		xor dx ,dx ; 将高位置为 0
		div bx 	   ; dx:ax / bx 结果  dx 为余数，ax 为商
		mov cx,ax  ; 商保存在 cx
		or  cx,dx   ; 如果 除数与被除数都为 0 直接跳转到 结束
		jz show_num_print
		add dx,'0'
		push dx	   ; 保存余数
		jmp show_num_loop
	show_num_0_:
			push  '0'	
	show_num_print:
		mov cx ,di 
		test si ,si 
		jz sho_num_print_loop
		push '-'
		sho_num_print_loop: 
			pop dx 
			mov si,sp
			mov ah ,0x0e ;功能号 显示字符 
			mov al ,dl 	 ; 字符
			mov bh ,0	; 页号
			mov bl ,0	; 前景色
			int 10h
			cmp sp,cx
			jne sho_num_print_loop
		pop di 
		pop bx 	
		pop si 
		leave
		ret



; 字符串比较
strcmp:
	push bp 
	mov bp,sp 
	push si 
	push di 
	mov si ,[bp+4]
	mov di ,[bp+6]
	strcmp_loop:
		xor ax,ax
		xor cx,cx 
		mov  al,[si]
		mov  cl,[di]
		sub ax,cx
		jnz strcmp_end
		add si, 1
		add di ,1 
		jmp strcmp_loop
	strcmp_end:	
		pop di 
		pop si 
		leave
		ret

; 返回字符串长度
; 参数
; str  word 字符串地址 :  [bp+4]
; 返回值
; ax=字符串长度
strlen:
	push bp
	mov bp, sp 
	push si
	push bx
	mov  bx ,[bp+4]
	mov si,0
	
	strlen_start:
		cmp byte [bx+si],0
		je strlen_end
		inc si
		jmp strlen_start

	strlen_end:
		mov ax,si
		pop si
		pop bx
		leave
		ret 



; 显示字符串函数 showmsg

; 参数 :

; word str  字符基址 
; word len  字符串长度

; 覆盖的寄存器,  ax,cx,dx



showmsg:
		push bp 
		mov  bp ,sp ; 现在的 bp 相当于 call 之前的 sp -4
		sub  sp ,4
		push bx
		push es

		mov ax,0
		mov es,ax		; 字符串段基址
		
		mov ah,0x13 	; 功能号 
		mov al,0x01 	; 光标方式，放到串尾

		mov bh,0		; 页号
		mov bl,0x0c  	; 属性 (0x0c 为黑底红字， 0xfh 为黑底白字 )

		mov dh,0	; 行号 
		mov dl,0	; 列号

		mov cx, [bp+6]	; 字符串长度
		
		push bp 		; 保存 bp 
		mov  bp, [bp+4]   ; 字符串地址
		
		int 10h
		
		pop bp 
		pop bx 
		pop es

		leave ; 16 位下相当于  mov sp,bp  然后  pop bp 
		ret 


showmsg3:
		push bp 
		mov  bp ,sp ; 现在的 bp 相当于 call 之前的 sp -4
		sub  sp ,4
		push bx
		push es

		mov ax,0
		mov es,ax		; 字符串段基址
		
		mov ah,0x13 	; 功能号 
		mov al,0x01 	; 光标方式，放到串尾

		mov bh,1		; 页号
		mov bl,0x0c  	; 属性 (0x0c 为黑底红字， 0xfh 为黑底白字 )

		mov dh,0	; 行号 
		mov dl,0	; 列号

		mov cx, [bp+6]	; 字符串长度
		
		push bp 		; 保存 bp 
		mov  bp, [bp+4]   ; 字符串地址
		
		int 10h
		
		pop bp 
		pop bx 
		pop es

		leave ; 16 位下相当于  mov sp,bp  然后  pop bp 
		ret 


; 显示字符串函数 showmsg2

; 参数 :

; word str  字符基址 
; word len  字符串长度
; word 行列号  高位 行号， 低位 列号

; 覆盖的寄存器,  ax,cx,dx



showmsg2:
		push bp 
		mov  bp ,sp ; 现在的 bp 相当于 call 之前的 sp -4
		sub  sp ,4
		push bx
		push es

		mov ax,0
		mov es,ax		; 字符串段基址
		
		mov ah,0x13 	; 功能号 
		mov al,0x01 	; 光标方式，放到串尾

		mov bh,0		; 页号
		mov bl,0x0c  	; 属性 (0x0c 为黑底红字， 0xfh 为黑底白字 )

		mov dx, [bp+8]	; 行列号

		mov cx, [bp+6]	; 字符串长度
		
		push bp 		; 保存 bp 
		mov  bp, [bp+4]   ; 字符串地址
		
		int 10h
		
		pop bp 
		pop bx 
		pop es

		leave ; 16 位下相当于  mov sp,bp  然后  pop bp 
		ret 

; 清除屏幕 clear 

clear:
	push bp 
	mov bp,sp 
	sub sp,0
	push bx
	mov ah, 6 ; 功能号
	mov al, 0 ; 滚动的文本行数（0=整个窗口）
	mov bh, 0fh ; 设置插入空行的字符颜色为黑底亮白字
	mov ch, 0 ; CH=行号、CL=列号
	mov cl, 0 ; 窗口左上角的行列号都为0
	mov dh, 24 ; 窗口右下角的行号，文本屏幕25行，行号=0~24
	mov dl, 79 ; 窗口右下角的列号，文本屏幕80列，列号=0~79
	int 10h ; 显示中断
	pop bx 
	leave
	ret 


; 向上滚动一行
scrollUp:
	push bp 
	mov bp,sp 
	sub sp,0
	push bx
	mov ah, 6 ; 功能号
	mov al, 1 ; 滚动的文本行数（0=整个窗口）
	mov bh, 0fh ; 设置插入空行的字符颜色为黑底亮白字
	mov ch, 0 ; CH=行号、CL=列号
	mov cl, 0 ; 窗口左上角的行列号都为0
	mov dh, 24 ; 窗口右下角的行号，文本屏幕25行，行号=0~24
	mov dl, 79 ; 窗口右下角的列号，文本屏幕80列，列号=0~79
	int 10h ; 显示中断
	pop bx 
	leave
	ret 

; 向下滚动一行
scrollDown:
	push bp 
	mov bp,sp 
	sub sp,0
	push bx
	mov ah, 7 ; 功能号
	mov al, 1 ; 滚动的文本行数（0=整个窗口）
	mov bh, 0fh ; 设置插入空行的字符颜色为黑底亮白字
	mov ch, 0 ; CH=行号、CL=列号
	mov cl, 0 ; 窗口左上角的行列号都为0
	mov dh, 24 ; 窗口右下角的行号，文本屏幕25行，行号=0~24
	mov dl, 79 ; 窗口右下角的列号，文本屏幕80列，列号=0~79
	int 10h ; 显示中断
	pop bx 
	leave
	ret 
