;----------读取磁盘测试-----------------
; hello-os
; TAB=4


CYLS	EQU		10				; 定义常量

		ORG		0x7c00			; 程序的装载地址

; 下面是标准FAT12格式软盘专用的代码

		JMP		entry
		DB		0x90
		DB		"HELLOIPL"		; 启动区名称 8 字节
		DW		512				; 每个扇区的大小，必须 512 字节
		DB		1				; 簇(cluster)的大小，必须一个扇区
		DW		1				; FAT 的起始位置，一般第一个扇区开始
		DB		2				; FAT 的个数，必须为 2
		DW		224				; 根目录的大小，一般224项
		DW		2880			; 该磁盘的大小，必须是2880扇区
		DB		0xf0			; 磁盘的种类，必须是 0xf0
		DW		9				; FAT 的长度，必须是 9 扇区
		DW		18				; 一个磁道(track)有几个扇区,必须是18
		DW		2				; 磁头数，必须是 2
		DD		0				; 不使用分区，必须是 0
		DD		2880			; ‚重写一次磁盘大小
		DB		0,0,0x29		; ‚意义不明，固定
		DD		0xffffffff		; (可能是) 卷标号码
		DB		"HELLO-OS   "	; 磁盘名称 11字节
		DB		"FAT12   "		; 磁盘格式名称 8 字节
		RESB	18				; 先空出 18 字节

; 程序核心


entry:
		MOV		AX,0			; ƒŒƒWƒXƒ^‰Šú‰»
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX

		MOV SI,smsg				; 读取成功
		JMP putloop	


putloop:
		MOV		AL,[SI]
		ADD		SI,1			; SI+1
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			; 显示一个文字
		MOV		BX,15			; 指定字符颜色
		INT		0x10			; 调用显卡BIOS
		JMP		putloop

fin:
		HLT						; CPU 停止，等待指令
		JMP		fin				; 无限循环


smsg:
		DB "load success"
		DB 0x0a 
		DB 0

		;RESB	0x7dfe-$		; 如果从软盘启动，检查第一个扇区最后的位置（一个扇区512B)是不是 0xaa55
		times (510-($-$$)) DB 0

		DB		0x55, 0xaa
