#include "debug.h"
#include "print.h"
#include "interrupt.h"
void __printdebug(char *str, int line);

void panic_spin(char *filename, int line, const char *func, const char *condition)
{
    intr_disable();
    print_debugln("<assert error>");
    print_debug_kv_str("fileName", filename);
    print_debug_kv_int("line", line);
    print_debug_kv_str("condition", condition);
    print_debugln("</assert error>");
    while (1)
    {
        ;
    }
}

void __printdebug(char *str, int line)
{
    put_str("[DEBUG:]  ");
    put_str(str);
    if (line)
    {
        put_char('\n');
    }
}

void print_debugln(char *str)
{
    __printdebug(str, 1);
}

void print_debug(char *str)
{
    __printdebug(str, 0);
}

void print_debug_kv_str(char *k, char *v)
{
    put_str("[DEBUG:]  ");
    put_str(k);
    put_str(" = : ");
    put_str(v);
    put_char('\n');
}

void print_debug_kv_int(char *k, uint32_t v)
{
    put_str("[DEBUG:]  ");
    put_str(k);
    put_str(" = :0x");
    put_int(v);
    put_char('\n');
}

void print_eip(uint32_t eip)
{
    print_debug_kv_int("eip", eip);
}

