TOOLPATH=../../../tools/
IMG=$(TOOLPATH)fat12/img 
MAKE=make 
COPY=cp 
DEL=rm
NASM=nasm
BOCHS=bochs

default:
	$(MAKE) img 

ipl.bin : ipl.asm Makefile
	$(NASM)  ipl.asm -o ipl.bin  

boot.img :os4.asm Makefile
	$(NASM) os4.asm -o boot.img 

os.img:boot.img ipl.bin Makefile
	$(IMG) -n os.img 
	$(IMG) -c os.img boot.img 
	$(IMG) -a os.img ipl.bin	

conf:
	$(COPY) $(TOOLPATH)bochs.conf bochs.conf

img :
	$(MAKE) os.img 

run:
	$(MAKE) img
	$(MAKE) conf
	$(BOCHS) -f bochs.conf

clean:
	-$(DEL) ipl.bin
	-$(DEL) boot.img

src_only:
	$(MAKE) clean
	-$(DEL) os.img 		