
cs_s.o:cs_s.asm
	nasm -f elf -o cs_s.o cs_s.asm
cs_c.o:cs_c.c
	gcc -o cs_c.o -c cs_c.c
cs.bin:cs_c.o cs_s.o
	ld -o cs.bin cs_c.o cs_s.o