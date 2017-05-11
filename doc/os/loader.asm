
%include "boot.inc"

org LOADER_BASE_ADDR


mov byte [gs:0x02],'o'
mov byte [gs:0x03],0xa4


mov byte [gs:0x04],'w'
mov byte [gs:0x05],0xa4

fin: 
	hlt
	jmp fin