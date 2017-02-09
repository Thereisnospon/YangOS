TOOLPATH=../../tools/
IMG=$(TOOLPATH)fat12/img 
MAKE=make 
COPY=cp 
DEL=rm
NASM=nasm
BOCHS=bochs

default:
	$(MAKE) img 


os.img:os.asm Makefile
	$(NASM) os.asm -o os.img 

conf:
	$(COPY) $(TOOLPATH)bochs.conf bochs.conf

img :
	$(MAKE) os.img 

run:
	$(MAKE) img
	$(MAKE) conf
	$(BOCHS) -f bochs.conf

# clean:
# 	-$(DEL) ipl.bin
# 	-$(DEL) boot.img

src_only:
	# $(MAKE) clean
	-$(DEL) os.img 		