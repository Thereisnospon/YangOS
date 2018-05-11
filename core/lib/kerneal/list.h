#ifndef __LIB_KERNEL_LIST_H
#define __LIB_KERNEL_LIST_H

#include "global.h"
#include "stdint.h"

/*
    offset 求出 struct type 类型的 member 成员相对于 结构体内部的偏移
    因为 &(struct_type_a.member) - &struct_type_a = n (偏移量)
    如果让 &struct_type_a = 0 ,那么  &(struct_type_a.member) = n 
    因此.   (&(struct_type*)0)->member = n 
*/
#define offset(struct_type, member) (int)(&((struct_type *)0)->member)

/*
    根据 struct_type 类型的 struct_member_name 成员的 elem_ptr 指针 反推出 它所在的结构体的 地址
    成员地址-成员在结构体偏移量= 结构体地址
*/
#define elem2entry(struct_type, struct_member_name, elem_ptr) \
    (struct_type *)((int)elem_ptr - offset(struct_type, struct_member_name))

struct list_elem
{
    struct list_elem *prev;
    struct list_elem *next;
};

struct list
{
    struct list_elem head;
    struct list_elem tail;
};

typedef bool(function)(struct list_elem *, int arg);

void list_init(struct list *);
void list_insert_before(struct list_elem *before, struct list_elem *elem);
void list_push(struct list *plist, struct list_elem *elem);
void list_iterate(struct list *plist);
void list_append(struct list *plist, struct list_elem *elem);
void list_remove(struct list_elem *pelem);
struct list_elem *list_pop(struct list *plist);
bool list_empty(struct list *plist);
uint32_t list_len(struct list *plist);
struct list_elem *list_traversal(struct list *plist, function func, int arg);
bool elem_find(struct list *plist, struct list_elem *obj_elem);
#endif
