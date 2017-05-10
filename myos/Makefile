
# 工具
TOOLPATH=../tools/
IMG=$(TOOLPATH)fat12/img 
MAKE=make 
COPY=cp 
DEL=rm
NASM=nasm
BOCHS=bochs


# 文件
OS=os.img
BOOT:=boot.asm
LDR:=loader.asm
KERNEL:=kernel.asm
BOOT_BIN:=$(subst .asm,.bin,$(BOOT))
LDR_BIN:=$(subst .asm,.bin,$(LDR))
KERNEL_BIN:=$(subst .asm,.bin,$(KERNEL))



default:
	$(MAKE) $(OS)


$(BOOT_BIN):$(BOOT)
	$(NASM) $< -o $@

$(LDR_BIN):$(LDR)	
	$(NASM) $< -o $@

$(KERNEL_BIN):
	$(NASM) -f bin -o $(KERNEL_BIN) $(KERNEL)

$(OS): $(BOOT_BIN) $(LDR_BIN) $(KERNEL_BIN) 
	$(IMG) -n $(OS)
	$(IMG) -c $(OS) $(BOOT_BIN)
	$(IMG) -a $(OS) $(LDR_BIN)
	$(IMG) -a $(OS) $(KERNEL_BIN)


conf:
	$(COPY) $(TOOLPATH)bochs.conf bochs.conf

run:
	$(MAKE) $(OS)
	$(MAKE) conf
	$(BOCHS) -f bochs.conf


clean:
	-$(DEL) *.bin
	-$(DEL) $(OS)






