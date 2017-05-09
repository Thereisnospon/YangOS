;foo.asm
extern choose 

;;;;;the data area 
num1st        dd        3 
num2nd        dd        4 

global        _start 
global        myprint 


_start: 

    push        dword [num1st] 
    push        dword [num2nd] 

    call    choose 
    add        esp,8 

    mov        ebx,0 
    mov        eax,1 
    int        0x80 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; 
    ;;;; function protype    :void myprint(char *msg, int len) 
    ;;;; display the message 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; 

myprint: 
     
    mov        ecx,[esp+4] 
    mov        edx,[esp+8] 
    ;mov        edx,esi 
    ;mov        ecx,edi 

    mov        ebx,1 
    mov        eax,4 
    int        0x80 
    ret