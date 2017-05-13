TOOLPATH=../tools/
IMG=$(TOOLPATH)fat12/img 
MAKE=make 
COPY=cp 
DEL=rm
NASM=nasm
BOCHS=bochs
BXIMAGE=bximage
SIZE=10
FILE=os.img 


ubuntu:
	make kerneal.bin 

kerneal.bin: kerneal.c 
		gcc -c -o kerneal.o kerneal.c
		ld kerneal.o -Ttext 0xc0001500 -e main -o kerneal.bin


loader.bin :loader.asm boot.inc 
	 $(NASM) loader.asm -o loader.bin

os.img:boot.bin loader.bin 
	touch os.img 
	rm os.img 
	$(BXIMAGE) -hd -mode='flat' -size=$(SIZE) -q  os.img 
	dd if=boot.bin of=os.img bs=512 count=1 conv=notrunc 
	dd if=loader.bin of=os.img bs=512 count=4 seek=2 conv=notrunc
	dd if=kerneal.bin of=os.img bs=512 count=200 seek=9 conv=notrunc

boot.bin:boot.asm boot.inc
	$(NASM) boot.asm -o boot.bin 

run:
	$(MAKE) os.img 
	$(BOCHS) -f bochs.conf

clean:
	-$(DEL) *.bin 
	-$(DEL) *.img 

