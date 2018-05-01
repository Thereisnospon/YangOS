/**************	 机器模式   ***************
	 b -- 输出寄存器QImode名称,即寄存器中的最低8位:[a-d]l。
	 w -- 输出寄存器HImode名称,即寄存器中2个字节的部分,如[a-d]x。
	 HImode
	     “Half-Integer”模式，表示一个两字节的整数。 
	 QImode
	     “Quarter-Integer”模式，表示一个一字节的整数。 
*******************************************/
#ifndef __LIB_IO_H
#define __LIB_IO_H

#include "../lib/stdint.h"
//向端口 port 写入一个字节
static inline void outb(uint16_t port,uint8_t data){
/*
    对端口指定 N (N 为立即数约束，0~255)表示 0~255 ， d 表示用 dx 存储端口号,
    %b0 表示对应 al, %w1 表示对应 dx
*/
    asm volatile ("outb %b0,%w1" : : "a"(data),"Nd"(port));
}
//将 addr 处起始的 word_cnt 个字节写入端口 port
static inline void outsw(uint16_t port,const void * addr,uin32_t word_cnt){
/*
    + 表示限制为 既做输入又做输出
    outsw 把 ds:esi 处的 16 位 内容写入 port 端口 
    在设置段描述符时，已经将 es,es,ss 的段选择子设置为相同内容了，此时不用担心数据问题 （os为平台模型）
*/
    asm volatile ("cld; rep outsw" : "+S"(addr),"+c"(word_cnt) : "d"(port));
}
//将从端口 port 读入的一个字节返回
static inline uint8_t inb(uint16_t port){
    uint8_t data;
    asm volatile ("inb %w1,%b0" : "=a"(data) : "Nd"(port));
    return data;
}
//将从端口 port读入的 word_cnt 个字节写入 addr
static inline void insw(uint16_t port,void *addr,uin32_t word_cnt){
    /*
        insw 是将从端口 port 读入的 16 位内容写入 es:edi 指向的内存
        cld; 清空方向位
        rep 重复指令 ，每次执行完，edi +2（
                这里没用到esi。
                使用了 w 且清除方向位. 所以每次 +2
        ). 
                重复 ecx 次。 
                每次执行一次 ecx 自动 -1
        D:是寄存器约束 edi
        d:是寄存器约束 dx
        c:是寄存器约束 ecx
        +:既做输入也做输出
        在设置段描述符时，已经将 es,es,ss 的段选择子设置为相同内容了，此时不用担心数据问题 （os为平台模型）
    */
    asm volatile ("cld; rep insw":"+D"(addr),"+c"(word_cnt):"d"(port):"memory");
}
#endif