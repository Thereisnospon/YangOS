TOOLPATH=../../../tools/
IMG=img 
MAKE=make 
COPY=cp 
DEL=rm
NASM=nasm
BOCHS=bochs

IPL=ipl 
OS=os 



default:
	$(MAKE) img 

$(IPL).bin : $(IPL).asm Makefile
	$(NASM)  $(IPL).asm -o $(IPL).bin  

boot.img :$(OS).asm Makefile
	$(NASM) $(OS).asm -o boot.img 

$(OS).img:boot.img $(IPL).bin Makefile
	$(IMG) -n $(OS).img 
	$(IMG) -c $(OS).img boot.img 
	$(IMG) -a $(OS).img $(IPL).bin	

conf:
	$(COPY) $(TOOLPATH)bochs.conf bochs.conf

img :
	$(MAKE) $(OS).img 

run:
	$(MAKE) img
	$(MAKE) conf
	$(BOCHS) -f bochs.conf

clean:
	-$(DEL) $(IPL).bin
	-$(DEL) boot.img

src_only:
	$(MAKE) clean
	-$(DEL) $(OS).img 		