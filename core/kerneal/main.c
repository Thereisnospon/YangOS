#include "print.h"
#include "init.h"
#include "debug.h"
// void test(){
//     put_int(0x00a);
//     put_char('\n');
//     put_int(0xab);
//     put_char('\n');
//     put_int(0x0);
//     put_char('\n');
//     put_str("gogo\n");
//     put_char('5');
//     put_char('k');
//     put_char('e');
//     put_char('r');
//     put_char('n');
//     put_char('e');
//     put_char('a');
//     put_char('l');
//     put_char('\n');
//     put_char('1');
//     put_char('2');
//     put_char('\b');
//     put_char('3');
//     put_char('4');
// }
void main(void){
   clean_screen();
    put_str("I am kerneal\n");
     init_all();
    ASSERT(1==2);
     //asm volatile("sti"); //临时打开中断
    while(1){
        ;
    }
}