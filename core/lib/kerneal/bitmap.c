#include "bitmap.h"
#include "stdint.h"
#include "string.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"

//位图btmp初始化
void bitmap_init(struct bitmap *btmp)
{
    memset(btmp->bits, 0, btmp->btmp_bytes_len);
}

//判断 bit_idx 位是否为1
bool bitmap_scan_test(struct bitmap *btmp, uint32_t bit_idx)
{
    uint32_t byte_idx = bit_idx / 8; //向下取整，用于索引数组下标
    uint32_t bit_odd = bit_idx % 8;  //取余索引数组内的位数
    //BITMAP_MASK（1） 左移 bit_odd 位，然后与 一个字节 做按位与，获取字节的 bit_odd 位是否为 1
    return (btmp->bits[byte_idx] & (BITMAP_MASK << bit_odd));
}

//在位图申请连续cnt个位，成功返回其下标，失败返回 -1
int bitmap_scan(struct bitmap *btmp, uint32_t cnt)
{
    uint32_t idx_byte = 0;
    //蛮力逐字节比较
    while ((0xff == btmp->bits[idx_byte]) && (idx_byte < btmp->btmp_bytes_len))
    {
        idx_byte++;
    }
    ASSERT(idx_byte < btmp->btmp_bytes_len);
    if (idx_byte == btmp->btmp_bytes_len)
    { //该地址内找不到可用空间
        return -1;
    }

    //找到有空闲位的字节，然后逐位对比
    int idx_bit = 0;
    while ((uint8_t)(BITMAP_MASK << idx_bit) & btmp->bits[idx_byte])
    {
        idx_bit++;
    }

    int bit_idx_start = idx_byte * 8 + idx_bit; //空闲位在位图下标
    if (cnt == 1)
    {
        return bit_idx_start;
    }

    uint32_t bit_left = (btmp->btmp_bytes_len * 8 - bit_idx_start);
    //记录还有多少位可以判断
    uint32_t next_bit = bit_idx_start + 1;
    uint32_t count = 1; //用于记录找到空闲位的个数

    bit_idx_start = -1; //先置其为-1，找不到连续的位就直接返回。

    while (bit_left-- > 0)
    {
        if (!(bitmap_scan_test(btmp, next_bit)))
        { //若next_bit为0
            count++;
          
            // put_int(count);
        }
        else
        {
            count = 0;
        }
        if (count == cnt)
        { //若找到连续的cnt个空位
            bit_idx_start = next_bit - cnt + 1;
            break;
        }
        next_bit++;
    }
    return bit_idx_start;
}

//位图的 bit_idx 位设置为 value
void bitmap_set(struct bitmap *btmp, uint32_t bit_idx, int8_t value)
{
    ASSERT((value == 0) || (value == 1));
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd = bit_idx % 8;

    if (value)
    {
        //设置为 1（非0）  byte | 0x00010000
        btmp->bits[byte_idx] |= (BITMAP_MASK << bit_odd);
    }
    else
    {
        //设置为 0  byte & 0x11101111
        btmp->bits[byte_idx] &= ~(BITMAP_MASK << bit_odd);
    }
}

void _test_print_bitmap(struct bitmap *btmp, int max)
{
    int i = 0;
    put_str("bitmap:(start=0x");
    put_int(btmp->bits);
    put_str(",len=0x");
    put_int(btmp->btmp_bytes_len);
    put_str(")\n");
    for (i = 0; i < btmp->btmp_bytes_len && i < max; i++)
    {
        put_str("0x");
        uint8_t num = btmp->bits[i];
        if (num <= 0xf)
        {
            put_char('0');
        }
        put_int(num);
        if ((i + 1) % 4 == 0)
        {
            put_char('\n');
        }
        else
        {
            put_char(' ');
        }
    }
    put_char('\n');
}

