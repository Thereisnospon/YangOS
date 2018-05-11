#ifndef __KERNEAL_DEBUG_H
#define __KERNEAL_DEBUG_H
#include "stdint.h"
#include "print.h"
void panic_spin(char *filename, int line, const char *func, const char *condition);
void print_eip(uint32_t eip);
uint32_t get_eip();
/***************************  __VA_ARGS__  *******************************
 * __FILE__ 编译的文件名
 * __LINE__ 编译文件中的行号
 * __func__ 被编译的函数名
 * __VA_ARGS__ 是预处理器所支持的专用标识符。
 * 代表所有与省略号相对应的参数. 
 * "..."表示定义的宏其参数可变.*/
#define PANIC(...) panic_spin(__FILE__, __LINE__, __func__, __VA_ARGS__)
/***********************************************************************/
static uint32_t __KERNEAL_DEBUG_H_eip = 0;
#ifdef NDEBUG
#define ASSERT(CONDITION) ((void)0)
#define PRINT_EIP \
    {             \
    }
#define EIP_START \
    {             \
    }
#define EIP_END \
    {           \
    }
#else
#define EIP_START                          \
    {                                      \
        __KERNEAL_DEBUG_H_eip = get_eip(); \
    }
#define EIP_END                           \
    {                                     \
        print_eip(__KERNEAL_DEBUG_H_eip); \
        __KERNEAL_DEBUG_H_eip = 0;        \
    }
#define PRINT_EIP                 \
    {                             \
        uint32_t eip = get_eip(); \
        print_eip(eip);           \
    }
#define ASSERT(CONDITION)                                                                              \
    EIP_START;                                                                                         \
    if (CONDITION)                                                                                     \
    {                                                                                                  \
    }                                                                                                  \
    else                                                                                               \
    {                                                                                                  \
        EIP_END;                                                                                       \
        /* 符号# 让编译器将宏的参数转化为字符串字面量 比如 a!=0 变为 "a!=0" */ \
        PANIC(#CONDITION);                                                                             \
    }
#endif
void print_debugln(char *str);
void print_debug(char *str);
void print_debug_kv_str(char *k, char *v);
void print_debug_kv_int(char *k, uint32_t v);
#endif