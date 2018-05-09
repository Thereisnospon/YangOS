#ifndef __LIB_KERNEAL_BITMAP_H
#define __LIB_KERNEAL_BITMAP_H
#include "global.h"
#include "stdint.h"
#define BITMAP_MASK 1
struct bitmap{
    uint32_t btmp_bytes_len;
    //遍历位图时，整体上以字节为单位，细节以位为单位，此处位图指针为单字节。
    uint8_t*bits;
};
void bitmap_init(struct bitmap*btmp);
bool bitmap_scan_test(struct bitmap*btmp,uint32_t bit_idx);
int bitmap_scan(struct bitmap*btmp,uint32_t cnt);
void bitmap_set(struct bitmap*btmp,uint32_t bit_idx,int8_t value);
void _test_print_bitmap(struct bitmap *btmp, int max);
#endif 