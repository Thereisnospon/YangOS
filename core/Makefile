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



loader.bin :loader.asm boot.inc 
	 $(NASM) loader.asm -o loader.bin

os.img:boot.bin loader.bin 
	touch os.img 
	rm os.img 
	$(BXIMAGE) -hd -mode='flat' -size=$(SIZE) -q  os.img 
	dd if=boot.bin of=os.img bs=512 count=1 conv=notrunc 
	dd if=loader.bin of=os.img bs=512 count=4 seek=2 conv=notrunc

boot.bin:boot.asm boot.inc
	$(NASM) boot.asm -o boot.bin 

run:
	$(MAKE) os.img 
	$(BOCHS) -f bochs.conf

clean:
	-$(DEL) *.bin 
	-$(DEL) *.img 

