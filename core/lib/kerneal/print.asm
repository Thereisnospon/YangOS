TI_GDT equ 0
REPL0 equ 0
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT +REPL0

[bits 32]

section .data 
put_int_buffer dq 0    


section .text
;----------------put_char--------------
;功能描述:把栈中一个字符写入光标所在处
;--------------------------------------
global put_char
put_char:
    pushad      ;备份32位寄存器环境

    ;保险起见，每次都必须保证 gs 为视屏段选择子
    mov ax,SELECTOR_VIDEO
    mov gs,ax

;;获取当前光标位置

    ; CRT Controller Registers 中的:
    ; Address Register : 3x4h
    ; Data Register : 3x5h
    ;0eh: Cursor Location High Register
    ;0fh: Cursor Location Low Retister

    ;先获取高8位
    mov dx,0x03d4   ; address Register
    mov al,0x0e     ; 高 8 位
    out dx,al       ; 将需要读取的地址写到 端口dx
    mov dx,0x03d5   ; data register
    in al,dx        ; 从端口获取到高 8 位数据
    mov ah,al       ; 高8位放到 ah

    ;获取低8位，与高8位类似
    mov dx,0x03d4   
    mov al,0x0f       ;低8位
    out dx,al
    mov dx,0x03d5
    in al,dx 

    ;光标放到 bx
    mov bx,ax 
    ;在栈中获取要打印的字符
    mov ecx ,[esp+36] ; pushad 压入 4*8=32
                      ; 返回地址 4， esp+36 是传入的参数
    ; cl 是 ecx 低 8 位
    cmp cl,0xd        ; CR=0x0d 回车符
    jz .is_carriage_return
    cmp cl,0xa        ; LF=0x0a 换行符
    jz .is_line_feed 

    cmp cl,0x8        ; BS=0x08 退格符
    jz .is_backspace  
    jmp .put_other    ; 普通打印字符

.is_backspace:
    dec bx                  ;减一，代表前一个位置（需要退格的位置)
                            ;显存中，一个字符位置用两个字符表示: 属性+ASCII
    shl bx,1                ;光标左移1位，等于乘以 2 
    mov byte [gs:bx],0x20   ;补空格符
    inc bx                  ;指向了显存位置的高字节处 ，即属性
    mov byte [gs:bx],0x07   ;字符属性，黑底白字
    shr bx,1                ;之前bx左移+1了，现在是奇数，右移一位舍弃余数。bx 回到了之前的光标位置
    jmp .set_cursor         ;调整光标位置

.put_other:
    shl bx,1                ;光标位置乘以2，对应于显存中对应的低字节
    mov [gs:bx],cl          ;需要显示的 ASCII符
    inc bx                  ;对应于字符属性
    mov byte [gs:bx],0x07        ;黑底白字
    shr bx,1                ;恢复光标值
    inc bx  ;下一个光标值
    cmp bx,2000
    jl .set_cursor          ;若光标位置小于 2000，表示未到显存最后
                            ;那么去设置新的光标位置
                            ;若超出屏幕字符大小(2000)
                            ;则换行处理

.is_line_feed:              ;换行符
.is_carriage_return:        ;回车符

    xor dx,dx       ;dx 是被除数的高16位，清0
    mov ax,bx       ;ax 是被除数的低16位
                    ;\n \r 都视为 下一行的行首
    mov si,80       
    
    div si          ;[高:dx,低:ax] / 80,  dx=余数
                    ;一行是80字符，光标值%80= 当前行的当前列数
    sub bx,dx       ;当前光标值-当前列数=当前行的行首光标处

.is_carriage_return_end:
    add bx,80       ;一行80列，光标在当前行的行首，加80就是下一行的行首了.
    cmp bx,2000     ;一屏幕:25行,一行80列 80*25 的2000 ，如果超过一屏幕需要滚屏

.is_line_feed_end:
    jl .set_cursor  ;不需要滚屏，设置光标位置

;屏幕行范围是0~24,滚屏的原理是 1~24 行移动到 0~23行;再将 24 行用空格填充
.roll_screen:
    cld
    mov ecx,960         ;2000-80 =1920 个字符需要移动，1920*2 =3840字节需要搬动
                        ;一次搬运4字节，需要3840/4=960次

    mov esi,0xc00b80a0  ;第1行行首
    mov edi,0xc00b8000  ;第0行行首
    rep movsd           ;4字节为单位依次搬运

;;最后一行填充为空白
    mov ebx,3840        ;最后一行行首的字节为1920*2=3840    
    mov ecx,80          ;一行80字符，每次清空1个字符
    ;接着执行 .cls
.cls:
    mov word [gs:ebx],0x0720    ;0x0720 黑底白字的空格符
    add ebx,2
    loop .cls 
    mov bx,1920                 ;光标重新设置为最后一行行首
    ;接着执行 .set_cursor

.set_cursor:

    ;设置光标高8位，与获取光标位置高8位类似
    mov dx,0x03d4
    mov al,0x0e
    out dx,al 
    mov dx,0x03d5
    mov al,bh
    out dx,al 

    ;设置低8位
    mov dx,0x03d4
    mov al,0x0f
    out dx,al
    mov dx,0x03d5
    mov al,bl 
    out dx,al

.put_char_done:
    popad
    ret     

global set_cursor
set_cursor:

    pushad      ;备份32位寄存器环境

    mov ecx ,[esp+36] ; pushad 压入 4*8=32
                      ; 返回地址 4， esp+36 是传入的参数                     
    mov bx,cx 
      ;设置光标高8位，与获取光标位置高8位类似
    mov dx,0x03d4
    mov al,0x0e
    out dx,al 
    mov dx,0x03d5
    mov al,bh
    out dx,al 

    ;设置低8位
    mov dx,0x03d4
    mov al,0x0f
    out dx,al
    mov dx,0x03d5
    mov al,bl 
    out dx,al

    popad
    ret     

;-------clean_scrren------------
;   功能:清除屏幕，光标置为起始位置
;-------------------------------
global clean_screen
clean_screen:
    push ebx
    push ecx
    mov ebx,0       ;第一行行首   
    mov ecx,2000          ;一行80字符，每次清空1个字符
    ;接着执行 .cls_cl_scren

.cls_cl_scren:
    mov word [gs:ebx],0x0720    ;0x0720 黑底白字的空格符
    add ebx,2
    loop .cls_cl_scren

    push 0
    call set_cursor
    add esp ,4
    pop ecx
    pop ebx 
    ret
 
 ;------------put_str------------
 ;  功能:打印字符串
 ;  参数:字符串首地址 （压栈)
 ;  字符串以\0结尾
 ;--------------------------------
 global put_str
 put_str:
    push ebx        ;保存用到的两个寄存器
    push ecx        
    xor ecx,ecx     ;ecx清空
    mov ebx,[esp+12]    ;ebx,ecx ,返回地址。一共12.  获取字符串地址
.goon:
    mov cl,[ebx]    ;获取字符
    cmp cl,0        
    jz .str_over    ;如果是字符0，字符串结束
    push ecx        ;调用 put_str打印字符
    call put_char   
    add esp,4       ;回收 put_str 用的栈控件
    inc ebx         ;指向下一个字符位置
    jmp .goon       
.str_over:
    pop ecx
    pop ebx 
    ret 

;--------put_int------------
;
;--------------------------
global put_int
put_int:
    pushad 
    mov ebp,esp 
    mov eax,[ebp+4*9]   ;8个寄存器+1个返回地址=36 获取int参数
    mov edx,eax         ; int 参数放到 edx
    mov edi,7           ;在 put_int_buffer 起始偏移量
    mov ecx,8           ;一个十六进制符(0~F) 有 4 bit.
                        ; 32位的数字，需要 8 个 符号显示 例如:0x00ff00ff
    mov ebx,put_int_buffer

.16based_4bits:
    and edx,0x0000000f  ;清空edx高位，只留下了低4位
    cmp edx,9           ;数字0~9 和 a~f 处理成不同字符
    jg .is_A2F          
    add edx,'0'         ;数字0~9 直接加 '0' 就是 '0'~'9' 的 ASCII
    jmp .store 
.is_A2F:
    sub edx,10          ;a~f 减去 10 加上 'A' 就是对应的 ASCII
    add edx,'A'
;将每位数字转换为对应字符，按照类似 大端 顺序
;存储到缓冲区 put_int_buffer
;高位字符放在低地址，低位字符放在高地址，这样和大端字节序类似
.store:
;dl 中应该是 数字对应的 ascii 码
    mov [ebx+edi],dl
    dec edi     ;放到下一个缓冲区位置(放到缓存区位置的顺序，是从大到小，获取字符的顺序是从小到达)
    shr eax,4   ;获取到更高的4位
    mov edx,eax ;edx临时放置eax
    loop .16based_4bits ;继续处理下一个字符

;put_int_buffer已经全是字符
;打印前把高位去掉，如0001,换成0
.ready_to_print:
    inc edi,    ;此时edi为-1,加一变为0
.skip_prefix_0: 
    cmp edi,8   ;如果比较edi比较到了9个字符，说明全部为0
    je .full0
;跳转前缀的0
.go_on_skip:
    mov cl,[put_int_buffer+edi] ;取buffer字符 
    inc edi                     ;指向buffer下一个字符
    cmp cl ,'0'                 ;如果是0
    je .skip_prefix_0           ;跳转到处理0
    dec edi                     ;上面inc edi 指向了下一个字符，如果当前不是0，那么edi指向拨回
    jmp .put_each_num           ;依次打印字符

.full0:
    mov cl,'0'                  ;数字全0，只打印0

.put_each_num:
    push ecx            ;cl中为打印字符
    call put_char
    add esp,4           ;清空栈控件
    inc edi             ;指向下一个buffer
    mov cl,[put_int_buffer+edi] ;获取下一个字符的 
    cmp edi,8
    jl .put_each_num
    popad 
    ret 


global cls_screen
cls_screen:
   pushad
   ;;;;;;;;;;;;;;;
	; 由于用户程序的cpl为3,显存段的dpl为0,故用于显存段的选择子gs在低于自己特权的环境中为0,
	; 导致用户程序再次进入中断后,gs为0,故直接在put_str中每次都为gs赋值. 
   mov ax, SELECTOR_VIDEO	       ; 不能直接把立即数送入gs,须由ax中转
   mov gs, ax

   mov ebx, 0
   mov ecx, 80*25
 .cls:
   mov word [gs:ebx], 0x0720		  ;0x0720是黑底白字的空格键
   add ebx, 2
   loop .cls 
   mov ebx, 0

 .set_cursor:				  ;直接把set_cursor搬过来用,省事
;;;;;;; 1 先设置高8位 ;;;;;;;;
   mov dx, 0x03d4			  ;索引寄存器
   mov al, 0x0e				  ;用于提供光标位置的高8位
   out dx, al
   mov dx, 0x03d5			  ;通过读写数据端口0x3d5来获得或设置光标位置 
   mov al, bh
   out dx, al

;;;;;;; 2 再设置低8位 ;;;;;;;;;
   mov dx, 0x03d4
   mov al, 0x0f
   out dx, al
   mov dx, 0x03d5 
   mov al, bl
   out dx, al
   popad
   ret