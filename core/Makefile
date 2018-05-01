#工具集路径
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
# docker api 项目的路径
DOCKER_CLIENT_PATH=$(TOOLPATH)docker-client/
#docker api 脚本。确保 contaienr 在后台的 Running 状态
DOCKER_CLIENT=$(DOCKER_CLIENT_PATH)docker-client.py
#运行docker api 脚本的 ptyon 解释器路径
PTYON=$(DOCKER_CLIENT_PATH)venv/bin/python

OUT_BUILD=build/
OUT_KERNEAL_BUILD=$(OUT_BUILD)kerneal/
OUT_KERNEAL_LIB_BUILD=$(OUT_KERNEAL_BUILD)lib/

OUT_F_LOADER_BIN=$(OUT_BUILD)loader.bin
OUT_F_KERNEAL_BIN=$(OUT_KERNEAL_BUILD)kerneal.bin



OUT_F_OS_IMG=$(OUT_BUILD)os.img
OUT_F_BOOT_BIN=$(OUT_BUILD)boot.bin

# 确保 32bit的 ubuntu docker 的 contaienr 是在后台 Running 的。
check_docker:
	$(PTYON) $(DOCKER_CLIENT) --src . --name osgo
#test
gk:
	make check_docker
	docker exec -it osgo make -f ubuntu-Makefile  go
#内核 已经移动动 ubuntu-32 的 docker 镜像运行
# kerneal.bin: kerneal.c 
# 		gcc -c -o kerneal.o kerneal.c
# 		# kerneal 设置起始虚拟地址为 0xc0001500
# 		ld kerneal.o -Ttext 0xc0001500 -e main -o kerneal.bin		

#loader
$(OUT_F_LOADER_BIN) :loader.asm boot.inc 
	 $(NASM) loader.asm -o $(OUT_F_LOADER_BIN)

$(OUT_F_KERNEAL_BIN):$(KERNEAL)*.c $(KERNEAL)*.h $(KERNEAL_LIB)*.c $(KERNEAL_LIB)*.h
	echo "gogo"
	docker exec -it osgo make -f ubuntu-Makefile  go
# 系统镜像
$(OUT_F_OS_IMG):$(OUT_F_BOOT_BIN) $(OUT_F_LOADER_BIN)
	rm -f $(OUT_F_OS_IMG)

	#强制
	docker exec -it osgo make -f ubuntu-Makefile  go

	$(BXIMAGE) -hd -mode='flat' -size=$(SIZE) -q  $(OUT_F_OS_IMG) 
	# MBR 放在 0 扇区
	dd if=$(OUT_F_BOOT_BIN) of=$(OUT_F_OS_IMG) bs=512 count=1 conv=notrunc 
	# 1 扇区空闲

	# loader 放在 第 2 扇区 开始。
	dd if=$(OUT_F_LOADER_BIN) of=$(OUT_F_OS_IMG) bs=512 count=4 seek=2 conv=notrunc
	# kerneal 程序放在第 9 扇区。（绕开0~8扇区)
	dd if=build/kerneal.bin  of=$(OUT_F_OS_IMG) bs=512 count=200 seek=9 conv=notrunc
#mbr
$(OUT_F_BOOT_BIN):boot.asm boot.inc
	$(NASM) boot.asm -o $(OUT_F_BOOT_BIN) 

#制作系统镜像，并用 bochs 启动
run:
	$(DEL) -f $(OUT_BUILD)*.img 
	$(DEL) -f $(OUT_BUILD)*.bin 
	$(MAKE) $(OUT_F_OS_IMG) 
	$(BOCHS) -f bochs.conf

clean:
	$(DEL) -rf $(OUT_BUILD)*.img 
	$(DEL) -rf $(OUT_BUILD)*.bin 
	$(DEL) -rf $(OUT_BUILD)*.o
	$(DEL) -rf $(OUT_KERNEAL_BUILD)*.o
	$(DEL) -rf $(OUT_KERNEAL_LIB_BUILD)*.o
	$(DEL) -rf $(OUT_KERNEAL_BUILD)*.bin 



