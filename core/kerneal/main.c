#include "print.h"
#include "init.h"
#include "debug.h"
#include "string.h"

void main(void){
    clean_screen();
    put_str("I am kerneal\n");
     init_all();
     char str1[100];
     char *go="hello";
     strcpy(str1,go);
     uint32_t len=strlen(str1);
     put_str(str1);
     put_int(len);
     put_str(go); 
    ASSERT(1==2);
     //asm volatile("sti"); //临时打开中断
    while(1){
        ;
    }
}