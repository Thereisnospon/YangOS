#include "string.h"
#include "global.h"
#include "assert.h"
/*
将dst_起始位置的 size 个字节设置为 value
*/
void memset(void *dst_, uint8_t value, uint32_t size)
{
    assert(dst_ != NULL);
    uint8_t *dst = (uint8_t *)dst_;
    while (size-- > 0)
        *dst++ = value;
}
/*
将 src 起始的 size 个字节 拷贝到 dst_
*/
void memcpy(void *dst_, const void *src_, uint32_t size)
{
    assert(dst_ != NULL && src_ != NULL);
    uint8_t *dst = dst_;
    const uint8_t *src = src_;
    while (size-- > 0)
        *dst++ = *src++;
}
/*
连续比较 地址 a 和 b 地址开头的 size 个字节，若相等返回 0 
*/
int memcmp(const void *a_, const void *b_, uint32_t size)
{
    const char *a = a_;
    const char *b = b_;
    assert(a != NULL || b != NULL);
    while(size-->0){
        if(*a!=*b){
            return *a > *b ? 1:-1;
        }
        a++;
        b++;
    }
    return 0;
}

/*
字符串 src_ 拷贝到 dst_
*/
char* strcpy(char*dst_,const char* src_){
   assert(dst_!=NULL && src_!=NULL);
   char* r=dst_;
   while((*dst_++=*src_++)){;}
   return r;
}
/*
返回字符串长度
*/
uint32_t strlen(const char* str){
    assert(str != NULL);
    const char *p=str;
    while(*p++){;}
    return (p-str-1);
}
/*
比较两个字符串， a_ 大于 b_ 返回 1，等于返回 0 ,小于返回 -1
*/
int8_t strcmp(const char*a,const char*b){
    assert(a != NULL && b != NULL);
    while(*a!=0 && *a==*b){
        a++;
        b++;
    }
    return *a<*b ? -1 : *a > *b ;
}
//从左到右查找出现 ch 的首次地址
char* strchar(const char*str,const uint8_t ch){
    assert(str != NULL);
    while(*str!=0){
        if(*str==ch){
            return (char*)str;
        }
        str++;
    }
    return NULL;
}
/*
从右往左查找首次出现 ch
*/
char* strrchr(const char*str,const uint8_t ch){
    assert(str != NULL);
    const char* last_char=NULL;
    while(*str!=0){
        if(*str==ch){
            last_char=str;
        }
        str++;
    }
    return (char*)last_char;
}
//拼接 src_到 dst_后
char * strcat(char * dst_,const char*src_){
    assert(dst_ != NULL && src_ != NULL);
    char *str=dst_;
    while(*str++);
    --str;
    while((*str++=*src_++));
    return dst_;
}
//str中查找ch出现的次数
uint32_t strchrs(const char*str,uint8_t ch){
    assert(str != NULL);
    uint32_t ch_cnt=0;
    const char*p=str;
    while(*p!=0){
        if(*p==ch){
            ch_cnt++;
        }
        p++;
    }
    return ch_cnt;
}