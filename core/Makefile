#*********************************************************************************
#  构建变量
#*********************************************************************************

BUILD_DIR=build
# 内核加载地址(虚拟地址)
ENTRY_POINT=0xc0001500
NASM=nasm 
GCC=gcc 
LD=ld
# python3 命令 路径。
PYTHON3=python3 
# python 工具的路径
PY_TOOLS=./build-tools
BUILD_TOOLS=./build-tools
#必须在 core 目录执行 make
C_DIR=$(shell pwd)
CONTAINER=osgo
#*********************************************************************************
#  c 代码编译 变量
#*********************************************************************************
INC_LIB= -I lib/ -I lib/kerneal -I lib/user -I kerneal/ -I device/ -I thread/ -I userprog/ -I fs/ -I shell/
ASFLAGS= -f elf 

# CFLAGS 说明
# -Wstrict-prototypes 函数声明必须要有参数类型
# -fno-builtin 不要用内建函数
# -Wmissing-prototypes 函数必须要有声明
# -fno-stack-protector 禁止栈保护
CFLAGS=-Wall $(INC_LIB) -c -fno-builtin -W -Wstrict-prototypes \
	   -Wmissing-prototypes -fno-stack-protector

# 链接标识	   
LDFLAGS=-Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kerneal.map 

# 目标文件
OBJS=$(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o \
		$(BUILD_DIR)/timer.o $(BUILD_DIR)/kerneal.o $(BUILD_DIR)/print.o \
		$(BUILD_DIR)/debug.o $(BUILD_DIR)/memory.o  $(BUILD_DIR)/bitmap.o \
		$(BUILD_DIR)/string.o $(BUILD_DIR)/thread.o  $(BUILD_DIR)/list.o \
		$(BUILD_DIR)/switch.o $(BUILD_DIR)/console.o $(BUILD_DIR)/sync.o \
		$(BUILD_DIR)/keyboard.o $(BUILD_DIR)/ioqueue.o $(BUILD_DIR)/tss.o \
		$(BUILD_DIR)/process.o $(BUILD_DIR)/syscall.o $(BUILD_DIR)/syscall-init.o \
		$(BUILD_DIR)/stdio.o $(BUILD_DIR)/ide.o $(BUILD_DIR)/stdio-kernel.o \
		$(BUILD_DIR)/fs.o $(BUILD_DIR)/inode.o $(BUILD_DIR)/file.o $(BUILD_DIR)/dir.o \
		$(BUILD_DIR)/fork.o  $(BUILD_DIR)/shell.o $(BUILD_DIR)/assert.o \
		$(BUILD_DIR)/buildin_cmd.o  $(BUILD_DIR)/exec.o $(BUILD_DIR)/wait_exit.o \
		$(BUILD_DIR)/pipe.o

OBJS_END=objs_end

#*********************************************************************************
#  c 代码编译
#*********************************************************************************
$(BUILD_DIR)/pipe.o: shell/pipe.c $(shell/pipe.c_hs)
	$(CC) $(CFLAGS) $< -o $@	

$(BUILD_DIR)/wait_exit.o : userprog/wait_exit.c $(userprog/wait_exit.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/assert.o : lib/user/assert.c $(lib/user/assert.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/exec.o: userprog/exec.c $(userprog/exec.c_hs)
	$(CC) $(CFLAGS) $< -o $@

 $(BUILD_DIR)/buildin_cmd.o : shell/buildin_cmd.c $(shell/buildin_cmd.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/shell.o :shell/shell.c $(shell/shell.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/fork.o : userprog/fork.c $(userprog/fork.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/inode.o:fs/inode.c $(fs/inode.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/file.o : fs/file.c $(fs/file.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/dir.o: fs/dir.c $(fs/dir.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/fs.o : fs/fs.c $(fs/fs.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/ide.o: device/ide.c $(device/ide.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/stdio-kernel.o: lib/kerneal/stdio-kernel.c $(lib/kerneal/stdio-kernel.c_hs) 
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/stdio.o: lib/stdio.c $(lib/stdio.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/syscall.o: lib/user/syscall.c $(lib/user/syscall.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/syscall-init.o: userprog/syscall-init.c $(userprog/syscall-init.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/process.o : userprog/process.c $(userprog/process.c_hs) 
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/tss.o : userprog/tss.c $(userprog/tss.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/ioqueue.o : device/ioqueue.c  $(device/ioqueue.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/keyboard.o: device/keyboard.c $(device/keyboard.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/console.o: device/console.c $(device/console.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/sync.o : thread/sync.c $(thread/sync.c_hs)
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/list.o: lib/kerneal/list.c $(lib/kerneal/list.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/thread.o: thread/thread.c $(thread/thread.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/memory.o : kerneal/memory.c $(kerneal/memory.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/bitmap.o: lib/kerneal/bitmap.c $(lib/kerneal/bitmap.c_hs)
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/string.o: lib/string.c $(lib/string.c_hs)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/main.o :kerneal/main.c $(kerneal/main.c_hs)
	$(GCC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o:kerneal/init.c $(kerneal/init.c_hs)
	$(GCC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: kerneal/interrupt.c $(kerneal/interrupt.c_hs)
	$(GCC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/timer.o : device/timer.c $(device/timer.c_hs)
	$(GCC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/debug.o : kerneal/debug.c $(kerneal/debug.c_hs)
	$(GCC) $(CFLAGS) $< -o $@

#*********************************************************************************
#  汇编 代码编译
#*********************************************************************************

$(BUILD_DIR)/kerneal.o : kerneal/kerneal.asm
	$(NASM) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/print.o : lib/kerneal/print.asm
	$(NASM) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/switch.o: thread/switch.asm
	$(NASM) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/start.o: command/start.asm
	$(NASM) $(ASFLAGS) $< -o $@

#  编译运行库 simple_crt.a
#*********************************************************************************

U_CFLAGS=-Wall -c -fno-builtin \
	-W -Wstrict-prototypes \
	-Wmissing-prototypes \
	-Wsystem-headers \
	-fno-stack-protector

U_LIBS=-I lib/ -I lib/kerneal/ \
	-I lib/user/ -I kerneal/ \
	-I device/ -I thread/ \
	-I userprog/ -I fs/ -I shell/

U_OBJS=$(BUILD_DIR)/string.o $(BUILD_DIR)/syscall.o \
       $(BUILD_DIR)/stdio.o  $(BUILD_DIR)/assert.o $(BUILD_DIR)/start.o

C_BUILD_F=$(U_CFLAGS) $(U_LIBS)
C_BUILD_DIR=$(BUILD_DIR)

#需要在容器里运行
$(BUILD_DIR)/simple_crt.a: $(U_OBJS)
	ar rcs $(BUILD_DIR)/simple_crt.a $(U_OBJS)

crt:
	docker exec -it $(CONTAINER) make -f  Makefile  $(BUILD_DIR)/simple_crt.a

S_CRT=$(BUILD_DIR)/simple_crt.a

$(BUILD_DIR)/pack_tool.bin :$(BUILD_TOOLS)/pack-tool.c
	gcc $(BUILD_TOOLS)/pack-tool.c -o $(BUILD_DIR)/pack_tool.bin

ALL_U_BIN=$(C_BUILD_DIR)/cat.bin $(C_BUILD_DIR)/prog_no_arg.bin  $(BUILD_DIR)/prog_arg.bin   $

$(BUILD_DIR)/prog-pack.img:$(BUILD_DIR)/pack_tool.bin $(ALL_U_BIN)
	$(BUILD_DIR)/pack_tool.bin create $(BUILD_DIR)/prog-pack.img $(ALL_U_BIN)

all_pack:$(BUILD_DIR)/prog-pack.img


# fil_add:$(BUILD_DIR)/pack_tool.bin $(ALL_U_BIN)
# 	$(BUILD_DIR)/pack_tool.bin append $(BUILD_DIR)/prog-pack.bin  $(BUILD_DIR)/tmp.txt 
	
#*********************************************************************************
# 编译用户程序
$(C_BUILD_DIR)/cat.o:command/cat.c $(command/cat.c_hs)
	gcc $(C_BUILD_F) $< -o $@

$(C_BUILD_DIR)/cat.bin:$(C_BUILD_DIR)/cat.o $(S_CRT)
	ld $< $(S_CRT) -o $@ 

$(C_BUILD_DIR)/prog_arg.o: command/prog_arg.c $(command/prog_arg.c_hs)
	gcc $(C_BUILD_F) $< -o $@

$(C_BUILD_DIR)/prog_arg.bin:$(C_BUILD_DIR)/prog_arg.o $(S_CRT)
	ld $< $(S_CRT) -o $@ 

$(C_BUILD_DIR)/prog_no_arg.o: command/prog_no_arg.c $(command/prog_arg.c_hs)
	gcc $(C_BUILD_F) $< -o $@

$(C_BUILD_DIR)/prog_no_arg.bin:$(C_BUILD_DIR)/prog_no_arg.o $(S_CRT)
	ld $< $(S_CRT) -o $@ 

#*********************************************************************************
#  链接
#*********************************************************************************

$(BUILD_DIR)/kerneal.bin : $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

#*********************************************************************************
#  引导程序编译
#*********************************************************************************

# loader
$(BUILD_DIR)/loader.bin :loader.asm boot.inc 
	$(NASM) $< -o $@

#mbr
$(BUILD_DIR)/boot.bin:boot.asm boot.inc
	$(NASM) $< -o $@

#*********************************************************************************
#  硬盘
#*********************************************************************************
#hd80.img 硬盘 （已经提前进行了分区的硬盘，避免每次手动分区,手动分区看 build-tools/分区.md）
$(BUILD_DIR)/hd80.img:
	rm -f $(BUILD_DIR)/hd80.img
	cp $(BUILD_TOOLS)/hd80.img $(BUILD_DIR)/hd80.img

unzip_hd80:
	rm -f $(BUILD_TOOLS)/hd80.img
	unzip $(BUILD_TOOLS)/hd80.img.zip -d $(BUILD_TOOLS)

#*********************************************************************************
#  生成系统镜像
#*********************************************************************************
# boot.bin : 磁盘引导
# loader.bin : 操作系统加载
# kerneal.bin : 内核
$(BUILD_DIR)/os.img:$(BUILD_DIR)/boot.bin $(BUILD_DIR)/loader.bin $(BUILD_DIR)/kerneal.bin 
	
	rm -f $(BUILD_DIR)/os.img

	# 内核所在的 硬盘（裸盘，没有文件系统） 大概 60mb
	bximage -hd -mode='flat' -size=60 -q  $(BUILD_DIR)/os.img 

	# MBR 放在 0 扇区
	dd if=$(BUILD_DIR)/boot.bin of=$(BUILD_DIR)/os.img bs=512 count=1 conv=notrunc 
	# 留 1 扇区空闲

	# loader 放在 第 2 扇区 开始。 
	dd if=$(BUILD_DIR)/loader.bin  of=$(BUILD_DIR)/os.img bs=512 count=4 seek=2 conv=notrunc
	# 内核放在第 9 扇区。（绕开0~8扇区)
	dd if=$(BUILD_DIR)/kerneal.bin of=$(BUILD_DIR)/os.img bs=512 count=200 seek=9 conv=notrunc

#*********************************************************************************
#  生成运行环境（如果没有开发工具，可以在docker 容器里build
#*********************************************************************************
build:$(BUILD_DIR)/os.img $(BUILD_DIR)/hd80.img $(BUILD_DIR)/prog-pack.img
	dd if=$(BUILD_DIR)/prog-pack.img of=$(BUILD_DIR)/os.img bs=512 \
	count=2000 seek=300 conv=notrunc
#*********************************************************************************
# 检索依赖的头文件
#*********************************************************************************
hs:
	$(PYTHON3) $(PY_TOOLS)/find_hs.py --scandir $(C_DIR) --u_src_mk $(C_DIR)/Makefile \
	--u_dst_mk $(C_DIR)/Makefile \
	--mode hs 

#在docker容器里执行 hs
hs_docker:
	docker exec -it $(CONTAINER) make -f Makefile hs

#*********************************************************************************
#  运行（需要安装bochs)
#*********************************************************************************
run:
	docker exec -it $(CONTAINER) make -f Makefile build
	bochs -f bochs.conf

#*********************************************************************************
#  启动编译需要的 docker 容器
#*********************************************************************************
container_run:
	docker run -d -P -it  \
	--name $(CONTAINER) \
	--mount type=bind,source=$(C_DIR),target=/opt/mydir \
	ubuntu:work-os32  bash

#*********************************************************************************
#  进入编译用的 container 容器 bash 环境
#*********************************************************************************
container_bash:
	docker exec -it $(CONTAINER) bash

DOCKER_PATH=$(BUILD_TOOLS)/docker-images

#*********************************************************************************
#  docker 初始化，编译两个需要的 image. (只需要初始化一次)
#*********************************************************************************
docker_init:
	docker build -t ubuntu:os-make -f $(DOCKER_PATH)/make-os/Dockerfile  $(DOCKER_PATH)/make-os
	docker build -t ubuntu:work-os32 -f $(DOCKER_PATH)/work-os32/Dockerfile  $(DOCKER_PATH)/work-os32

#*********************************************************************************
# 初始化编译环境
#*********************************************************************************
init: docker_init unzip_hd80
	if([[! -d $(BUILD_DIR)]]);then mkdir $(BUILD_DIR) ;fi  

.PHONY: docker_init container_bash container_run hs_docker hs unzip_hd80

EOF_EOF_EOF=eof_eof_eof
/command/cat.c_hs= /lib/user/syscall.h /lib/stdio.h /lib/string.h /kerneal/global.h /fs/fs.h
/command/prog_arg.c_hs= /lib/stdio.h /lib/user/syscall.h /lib/string.h
/command/prog_no_arg.c_hs= /lib/stdio.h
/command/prog_pipe.c_hs= /lib/stdio.h /lib/user/syscall.h /lib/string.h
/device/console.c_hs= /device/console.h /lib/kerneal/print.h /lib/stdint.h /thread/sync.h /thread/thread.h
/device/ide.c_hs= /device/ide.h /thread/sync.h /lib/stdio.h /lib/kerneal/stdio-kernel.h /kerneal/interrupt.h /kerneal/memory.h /kerneal/debug.h /lib/string.h /lib/kerneal/io.h
/device/ioqueue.c_hs= /device/ioqueue.h /kerneal/interrupt.h /kerneal/global.h /kerneal/debug.h
/device/keyboard.c_hs= /device/keyboard.h /lib/kerneal/print.h /kerneal/interrupt.h /lib/kerneal/io.h /kerneal/global.h /device/ioqueue.h
/device/timer.c_hs= /device/timer.h /lib/kerneal/io.h /lib/kerneal/print.h /thread/thread.h /kerneal/debug.h
/fs/dir.c_hs= /fs/dir.h /lib/stdint.h /fs/inode.h /fs/file.h /fs/fs.h /lib/kerneal/stdio-kernel.h /kerneal/global.h /kerneal/debug.h /kerneal/memory.h /lib/string.h /kerneal/interrupt.h /fs/super_block.h
/fs/file.c_hs= /fs/file.h /fs/fs.h /fs/super_block.h /fs/inode.h /lib/kerneal/stdio-kernel.h /kerneal/memory.h /kerneal/debug.h /kerneal/interrupt.h /lib/string.h /thread/thread.h /kerneal/global.h
/fs/fs.c_hs= /fs/fs.h /fs/super_block.h /fs/inode.h /fs/dir.h /lib/stdint.h /lib/kerneal/stdio-kernel.h /lib/kerneal/list.h /lib/string.h /device/ide.h /kerneal/global.h /kerneal/debug.h /kerneal/memory.h /fs/file.h /device/console.h /device/keyboard.h /device/ioqueue.h /shell/pipe.h
/fs/inode.c_hs= /fs/inode.h /fs/fs.h /fs/file.h /kerneal/global.h /kerneal/debug.h /kerneal/memory.h /kerneal/interrupt.h /lib/kerneal/list.h /lib/kerneal/stdio-kernel.h /lib/string.h /fs/super_block.h
/kerneal/debug.c_hs= /kerneal/debug.h /lib/kerneal/print.h /kerneal/interrupt.h
/kerneal/init.c_hs= /kerneal/init.h /lib/kerneal/print.h /kerneal/interrupt.h /kerneal/memory.h /device/timer.h /thread/thread.h /device/console.h /device/keyboard.h /userprog/tss.h /userprog/syscall-init.h /device/ide.h /fs/fs.h
/kerneal/interrupt.c_hs= /kerneal/interrupt.h /lib/stdint.h /kerneal/global.h /lib/kerneal/io.h /lib/kerneal/print.h /kerneal/debug.h
/kerneal/main.c_hs= /lib/kerneal/print.h /kerneal/init.h /kerneal/debug.h /lib/string.h /lib/kerneal/bitmap.h /kerneal/memory.h /device/console.h /device/ioqueue.h /device/keyboard.h /kerneal/interrupt.h /userprog/process.h /lib/user/syscall.h /userprog/syscall-init.h /lib/stdio.h /lib/kerneal/stdio-kernel.h /fs/fs.h /fs/dir.h /shell/shell.h
/kerneal/memory.c_hs= /kerneal/memory.h /lib/kerneal/bitmap.h /lib/stdint.h /kerneal/global.h /kerneal/debug.h /lib/kerneal/print.h /lib/string.h /thread/sync.h /kerneal/interrupt.h
/lib/kerneal/bitmap.c_hs= /lib/kerneal/bitmap.h /lib/stdint.h /lib/string.h /lib/kerneal/print.h /kerneal/interrupt.h /kerneal/debug.h
/lib/kerneal/list.c_hs= /lib/kerneal/list.h /kerneal/interrupt.h /lib/stdint.h /lib/kerneal/print.h /kerneal/debug.h
/lib/kerneal/stdio-kernel.c_hs= /lib/kerneal/stdio-kernel.h /lib/kerneal/print.h /lib/stdio.h /device/console.h /kerneal/global.h
/lib/stdio.c_hs= /lib/stdio.h /kerneal/interrupt.h /kerneal/global.h /lib/string.h /lib/user/syscall.h /lib/kerneal/print.h
/lib/string.c_hs= /lib/string.h /kerneal/global.h /lib/user/assert.h
/lib/user/assert.c_hs= /lib/user/assert.h /lib/stdio.h
/lib/user/syscall.c_hs= /lib/user/syscall.h /thread/thread.h
/shell/buildin_cmd.c_hs= /shell/buildin_cmd.h /lib/user/syscall.h /lib/stdio.h /lib/string.h /fs/fs.h /kerneal/global.h /fs/dir.h /shell/shell.h /kerneal/debug.h
/shell/pipe.c_hs= /shell/pipe.h /kerneal/memory.h /fs/fs.h /fs/file.h /device/ioqueue.h /thread/thread.h
/shell/shell.c_hs= /shell/shell.h /lib/stdint.h /fs/file.h /lib/user/syscall.h /lib/stdio.h /kerneal/global.h /lib/user/assert.h /lib/string.h /shell/buildin_cmd.h /shell/pipe.h
/thread/sync.c_hs= /thread/sync.h /lib/kerneal/list.h /kerneal/global.h /kerneal/debug.h /kerneal/interrupt.h
/thread/thread.c_hs= /thread/thread.h /lib/stdint.h /lib/string.h /kerneal/global.h /kerneal/debug.h /kerneal/interrupt.h /lib/kerneal/print.h /kerneal/memory.h /userprog/process.h /thread/sync.h /lib/stdio.h /fs/file.h /fs/fs.h
/userprog/exec.c_hs= /userprog/exec.h /thread/thread.h /lib/kerneal/stdio-kernel.h /fs/fs.h /lib/string.h /kerneal/global.h /kerneal/memory.h
/userprog/fork.c_hs= /userprog/fork.h /userprog/process.h /kerneal/memory.h /kerneal/interrupt.h /kerneal/debug.h /thread/thread.h /lib/string.h /fs/file.h /shell/pipe.h
/userprog/process.c_hs= /userprog/process.h /kerneal/global.h /kerneal/debug.h /kerneal/memory.h /thread/thread.h /lib/kerneal/list.h /userprog/tss.h /kerneal/interrupt.h /lib/string.h /device/console.h /kerneal/debug.h
/userprog/syscall-init.c_hs= /userprog/syscall-init.h /lib/user/syscall.h /lib/stdint.h /lib/kerneal/print.h /thread/thread.h /device/console.h /lib/string.h /kerneal/memory.h /fs/fs.h /userprog/fork.h /userprog/exec.h /userprog/wait_exit.h /shell/pipe.h
/userprog/tss.c_hs= /kerneal/debug.h /userprog/tss.h /lib/stdint.h /kerneal/global.h /lib/string.h /lib/kerneal/print.h
/userprog/wait_exit.c_hs= /userprog/wait_exit.h /kerneal/global.h /kerneal/debug.h /thread/thread.h /lib/kerneal/list.h /lib/kerneal/stdio-kernel.h /kerneal/memory.h /lib/kerneal/bitmap.h /fs/fs.h /fs/file.h



