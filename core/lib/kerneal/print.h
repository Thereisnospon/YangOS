#ifndef _LIB_KERNEAL_PRINT_H
#define _LIB_KERNEAL_PRINT_H
#include "stdint.h"
void put_char(uint8_t char_ascii);
void set_cursor(uint8_t pos);
void clean_screen(void);
void put_str(char *message);
void put_int(uint32_t num);
#endif