#工具集路径
TOOLPATH=../tools/
IMG=$(TOOLPATH)fat12/img 
MAKE=make 
COPY=cp 
DEL=rm
NASM=nasm
BOCHS=bochs
BXIMAGE=bximage
SIZE=60
FILE=os.img 
# docker api 项目的路径
DOCKER_CLIENT_PATH=$(TOOLPATH)docker-client/
#docker api 脚本。确保 contaienr 在后台的 Running 状态
DOCKER_CLIENT=$(DOCKER_CLIENT_PATH)docker-client.py
#运行docker api 脚本的 ptyon 解释器路径
PTYON=$(DOCKER_CLIENT_PATH)venv/bin/python
PYTHON3=/usr/local/bin/python3
OUT_BUILD=build/
OUT_KERNEAL_BUILD=$(OUT_BUILD)kerneal/
OUT_KERNEAL_LIB_BUILD=$(OUT_KERNEAL_BUILD)lib/
OUT_F_LOADER_BIN=$(OUT_BUILD)loader.bin
OUT_F_KERNEAL_BIN=$(OUT_KERNEAL_BUILD)kerneal.bin
OUT_F_OS_IMG=$(OUT_BUILD)os.img
OUT_F_BOOT_BIN=$(OUT_BUILD)boot.bin
C_DIR=/Users/yzr/repos/YangOS/core
META_FILES = meta-files
PY_TOOLS=$(TOOLPATH)py-tools

UB_MAKE_FILE=/Users/yzr/repos/YangOS/core/ubuntu-Makefile
MAKE_FILE=/Users/yzr/repos/YangOS/core/Makefile
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
$(OUT_F_OS_IMG):$(OUT_F_BOOT_BIN) $(OUT_F_LOADER_BIN) build/kerneal.bin 
	rm -f $(OUT_F_OS_IMG)
	
	#强制

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

build/kerneal.bin:$(OBJS)
	docker exec -it osgo make -f ubuntu-Makefile  go

#这里的 hd80 文件是用事先已经创建并分区号的文件。避免每次繁琐的分区过程。

$(OUT_BUILD)hd80.img:bochs.conf 
	$(DEL) -rf $(OUT_BUILD)hd80.img
	cp $(META_FILES)/hd80.img $(OUT_BUILD)hd80.img

prog_no_arg:
	docker exec -it osgo make -f ubuntu-Makefile  prog_no_arg

prog_arg:
	docker exec -it osgo make -f ubuntu-Makefile  prog_arg
cat:
	docker exec -it osgo make -f ubuntu-Makefile  cat
hd80:
	$(MAKE) $(OUT_BUILD)hd80.img
#制作系统镜像，并用 bochs 启动
run:
	rm -rf  build/kerneal.bin 
	$(DEL) -rf $(OUT_BUILD)hd80.img
	$(MAKE) $(OUT_F_OS_IMG) 
	$(MAKE) prog_no_arg
	$(MAKE) $(OUT_BUILD)hd80.img
	$(BOCHS) -f bochs.conf

run3:
	rm -rf  build/kerneal.bin 
	$(DEL) -rf $(OUT_BUILD)hd80.img
	cp $(META_FILES)/hd80_prog.img $(OUT_BUILD)hd80.img
	$(MAKE) $(OUT_F_OS_IMG) 
	$(MAKE) prog_arg
	$(BOCHS) -f bochs.conf

run4:	
	rm -rf  build/kerneal.bin 
	$(DEL) -rf $(OUT_BUILD)hd80.img
	cp $(META_FILES)/hd80_prog.img $(OUT_BUILD)hd80.img
	$(MAKE) $(OUT_F_OS_IMG) 
	$(MAKE) cat
	$(BOCHS) -f bochs.conf

clean:
	$(DEL) -rf $(OUT_BUILD)*.img 
	$(DEL) -rf $(OUT_BUILD)*.bin 
	$(DEL) -rf $(OUT_BUILD)*.o

#解析头文件依赖
hs:
	$(PYTHON3) $(PY_TOOLS)/find_hs.py --scandir $(C_DIR) --u_src_mk $(UB_MAKE_FILE)\
		--u_dst_mk $(UB_MAKE_FILE)\
		--g_src_mk $(MAKE_FILE)\
		--g_dst_mk $(MAKE_FILE)\
		--mode hs 
mobjs:
	$(PYTHON3) $(PY_TOOLS)/find_hs.py --scandir $(C_DIR) --u_src_mk $(UB_MAKE_FILE)\
		--u_dst_mk $(UB_MAKE_FILE)\
		--g_src_mk $(MAKE_FILE)\
		--g_dst_mk $(MAKE_FILE)\
		--mode objs 
all_gf:
	make hs
	make mobjs

#meta-files 里放置了创建好了分区表的硬盘文件
#解压它到meta-files 目录
unzip_hd80:
	rm -rf $(META_FILES)/hd80.img
	unzip $(META_FILES)/hd80.img.zip -d $(META_FILES)

#使用bximage 创建一个空的hd80.img 文件。需要安装下面的分区步骤一步一步分区
touch_hd80:
	$(BXIMAGE) -hd -mode='flat' -size=80 -q $(OUT_BUILD)hd80.img



#******************************************************************************************************
#******************************************************************************************************
#
#  分区步骤
#
# 在书上看 fdisk 分配主分区时，要求给起始柱面号 cylinders, 但是自己找的 fdisk 的各个版本都是 要求起始扇区号 sector.. 然后发现 fdisk 启动时候需要设置 模式
# http://wangchujiang.com/linux-command/c/fdisk.html  
#```
# fdisk -u=cylinders build/hd80.img
# 又遇到坑。 first cylinders 是 3 不是 1， 需要用到设置模式为 dos 兼容模式 -c=dos
# fdisk -u=cylinders -c=dos  build/hd80.img
# 一次性命令。 cylinders ,dos 兼容 柱面162个， 磁头16个
# fdisk -u=cylinders -c=dos  -C 162 -H 16  build/hd80.img
# 根据命令 fdisk -u=cylinders -c=dos  -C 162 -H 16  build/hd80.img 进入后

# STEP1 : 创建主分区 1
# Command (m for help): n  （ 按n 创建分区
# Partition type:
#    p   primary (0 primary, 0 extended, 4 free)
#    e   extended
# Select (default p): p （按 p 创建 主分区
# Partition number (1-4, default 1): 1 ( 按 1 设置分区号
# First cylinder (1-162, default 1): 1 ( 按 1 设置起始柱面号为 1
# Last cylinder, +cylinders or +size{K,M,G} (1-162, default 162): 32 （按32 设置终止柱面号为 32
# Partition 1 of type Linux and of size 15.7 MiB is set
# ```

# STEP2: 创建扩展分区 4
# Command (m for help): n （ 按n 创建分区
# Partition type:
#    p   primary (1 primary, 0 extended, 3 free)
#    e   extended
# Select (default p): e (按 e 创建 扩展分区
# Partition number (2-4, default 2): 4 ( 按 4 设置分区号
# First cylinder (33-162, default 33): 33 ( 按 33 设置起始柱面号为 33
# Last cylinder, +cylinders or +size{K,M,G} (33-162, default 162): 162 （按162 设置终止柱面号为 162
# Partition 4 of type Extended and of size 64 MiB is set

# STEP3: 按 p 查看下当前分区
# Command (m for help): p

# Disk build/hd80.img: 83 MB, 83607552 bytes, 163296 sectors
# 16 heads, 63 sectors/track, 162 cylinders
# Units = cylinders of 1008 * 512 = 516096 bytes
# Sector size (logical/physical): 512 bytes / 512 bytes
# I/O size (minimum/optimal): 512 bytes / 512 bytes
# Disk label type: dos
# Disk identifier: 0xf25071c7

#          Device Boot      Start         End      Blocks   Id  System
# build/hd80.img1               1          32       16096+  83  Linux
# build/hd80.img4              33         162       65520    5  Extended


#STEP4: 创建逻辑分区 (依次创建逻辑分区:)
# logig1: 33~50
# logic2: 51~75
# logic3: 76~90
# logic4: 91~120
# logic5: 121~162
# Command (m for help): n （ 按n 创建分区
# Partition type:
#    p   primary (1 primary, 1 extended, 2 free)
#    l   logical (numbered from 5)
# Select (default p): l （按 l 创建逻辑分区
# Adding logical partition 5 
# First cylinder (33-162, default 33): 33 ( 按 33 设置起始柱面号为 33
# Last cylinder, +cylinders or +size{K,M,G} (33-162, default 162): 50 （按162 设置终止柱面号为 50
# Partition 5 of type Linux and of size 8.8 MiB is set

# STEP5: 按 p 查看当前分区信息
# Disk build/hd80.img: 83 MB, 83607552 bytes, 163296 sectors
# 16 heads, 63 sectors/track, 162 cylinders
# Units = cylinders of 1008 * 512 = 516096 bytes
# Sector size (logical/physical): 512 bytes / 512 bytes
# I/O size (minimum/optimal): 512 bytes / 512 bytes
# Disk label type: dos
# Disk identifier: 0xf25071c7

#          Device Boot      Start         End      Blocks   Id  System
# build/hd80.img1               1          32       16096+  83  Linux
# build/hd80.img4              33         162       65520    5  Extended
# build/hd80.img5              33          50        9040+  83  Linux
# build/hd80.img6              51          75       12568+  83  Linux
# build/hd80.img7              76          90        7528+  83  Linux
# build/hd80.img8              91         120       15088+  83  Linux
# build/hd80.img9             121         162       21136+  83  Linux

#STEP6: 按 l 查看已知文件系统id
#  0  Empty           24  NEC DOS         81  Minix / old Lin bf  Solaris
#  1  FAT12           27  Hidden NTFS Win 82  Linux swap / So c1  DRDOS/sec (FAT-
#  2  XENIX root      39  Plan 9          83  Linux           c4  DRDOS/sec (FAT-
#  3  XENIX usr       3c  PartitionMagic  84  OS/2 hidden C:  c6  DRDOS/sec (FAT-
#  4  FAT16 <32M      40  Venix 80286     85  Linux extended  c7  Syrinx
#  5  Extended        41  PPC PReP Boot   86  NTFS volume set da  Non-FS data
#  6  FAT16           42  SFS             87  NTFS volume set db  CP/M / CTOS / .
#  7  HPFS/NTFS/exFAT 4d  QNX4.x          88  Linux plaintext de  Dell Utility
#  8  AIX             4e  QNX4.x 2nd part 8e  Linux LVM       df  BootIt
#  9  AIX bootable    4f  QNX4.x 3rd part 93  Amoeba          e1  DOS access
#  a  OS/2 Boot Manag 50  OnTrack DM      94  Amoeba BBT      e3  DOS R/O
#  b  W95 FAT32       51  OnTrack DM6 Aux 9f  BSD/OS          e4  SpeedStor
#  c  W95 FAT32 (LBA) 52  CP/M            a0  IBM Thinkpad hi eb  BeOS fs
#  e  W95 FAT16 (LBA) 53  OnTrack DM6 Aux a5  FreeBSD         ee  GPT
#  f  W95 Ext'd (LBA) 54  OnTrackDM6      a6  OpenBSD         ef  EFI (FAT-12/16/
# 10  OPUS            55  EZ-Drive        a7  NeXTSTEP        f0  Linux/PA-RISC b
# 11  Hidden FAT12    56  Golden Bow      a8  Darwin UFS      f1  SpeedStor
# 12  Compaq diagnost 5c  Priam Edisk     a9  NetBSD          f4  SpeedStor
# 14  Hidden FAT16 <3 61  SpeedStor       ab  Darwin boot     f2  DOS secondary
# 16  Hidden FAT16    63  GNU HURD or Sys af  HFS / HFS+      fb  VMware VMFS
# 17  Hidden HPFS/NTF 64  Novell Netware  b7  BSDI fs         fc  VMware VMKCORE
# 18  AST SmartSleep  65  Novell Netware  b8  BSDI swap       fd  Linux raid auto
# 1b  Hidden W95 FAT3 70  DiskSecure Mult bb  Boot Wizard hid fe  LANstep
# 1c  Hidden W95 FAT3 75  PC/IX           be  Solaris boot    ff  BBT
# 1e  Hidden W95 FAT1 80  Old Minix

#SETP7: 设置分区id （依次设置 5~9 的id为 66
# Command (m for help): t (键入t设置分区id
# Partition number (1,4-9, default 9): 5 (逻辑分区号5)
# Hex code (type L to list all codes): 66 （分区id 66 
# Changed type of partition 'Linux' to 'unknown'

#STEP8: 按p 检查分区表
# Disk build/hd80.img: 83 MB, 83607552 bytes, 163296 sectors
# 16 heads, 63 sectors/track, 162 cylinders
# Units = cylinders of 1008 * 512 = 516096 bytes
# Sector size (logical/physical): 512 bytes / 512 bytes
# I/O size (minimum/optimal): 512 bytes / 512 bytes
# Disk label type: dos
# Disk identifier: 0xf25071c7

#          Device Boot      Start         End      Blocks   Id  System
# build/hd80.img1               1          32       16096+  83  Linux
# build/hd80.img4              33         162       65520    5  Extended
# build/hd80.img5              33          50        9040+  66  Unknown
# build/hd80.img6              51          75       12568+  66  Unknown
# build/hd80.img7              76          90        7528+  66  Unknown
# build/hd80.img8              91         120       15088+  66  Unknown
# build/hd80.img9             121         162       21136+  66  Unknown

#STEP9: 按9 写入文件退出


EOF_EOF_EOF=eof_eof_eof
OBJS=$(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o \
		$(BUILD_DIR)/timer.o $(BUILD_DIR)/kerneal.o $(BUILD_DIR)/print.o \
		$(BUILD_DIR)/debug.o $(BUILD_DIR)/memory.o  $(BUILD_DIR)/bitmap.o \
		$(BUILD_DIR)/string.o $(BUILD_DIR)/thread.o  $(BUILD_DIR)/list.o $(BUILD_DIR)/switch.o \
		$(BUILD_DIR)/console.o $(BUILD_DIR)/sync.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/ioqueue.o $(BUILD_DIR)/tss.o \
		$(BUILD_DIR)/process.o $(BUILD_DIR)/syscall.o $(BUILD_DIR)/syscall-init.o $(BUILD_DIR)/stdio.o \
		$(BUILD_DIR)/ide.o $(BUILD_DIR)/stdio-kernel.o $(BUILD_DIR)/fs.o $(BUILD_DIR)/inode.o $(BUILD_DIR)/file.o $(BUILD_DIR)/dir.o \
		$(BUILD_DIR)/fork.o 
OBJS_END=objs_end
