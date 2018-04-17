#include "print.h"

void main(void){
    clean_screen();
    put_int(0x00a);
    put_char('\n');
    put_int(0xab);
    put_char('\n');
    put_int(0x0);
    put_char('\n');
    put_str("gogo\n");
    put_char('5');
    put_char('k');
    put_char('e');
    put_char('r');
    put_char('n');
    put_char('e');
    put_char('a');
    put_char('l');
    put_char('\n');
    put_char('1');
    put_char('2');
    put_char('\b');
    put_char('3');
    put_char('4');
    while(1){
        ;
    }
}