#include "list.h"
#include "interrupt.h"
#include "stdint.h"

//初始化双向链表
void list_init(struct list *list)
{
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
}

//链表元素 elem 放在 before 元素之前
void list_insert_before(struct list_elem *before, struct list_elem *elem)
{

    enum intr_status old_status = intr_disable();

    before->prev->next = elem;
    elem->prev = before->prev;
    elem->next = before;

    before->prev = elem;
    intr_set_status(old_status);
}

//添加元素到链表队首
void list_push(struct list *plist, struct list_elem *elem)
{
    list_insert_before(plist->head.next, elem);
}
//添加元素到链表队尾
void list_append(struct list *plist, struct list_elem *elem)
{
    list_insert_before(&plist->tail, elem);
}

//列表删除元素
void list_remove(struct list_elem *pelem)
{
    enum intr_status old_status = intr_disable();
    pelem->prev->next = pelem->next;
    pelem->next->prev = pelem->prev;
    intr_set_status(old_status);
}
//链表第一个元素弹出并返回
struct list_elem *list_pop(struct list *plist)
{
    struct list_elem *elem = plist->head.next;
    list_remove(elem);
    return elem;
}

//查找 obj_elem 
bool elem_find(struct list *plist, struct list_elem *obj_elem)
{
    struct list_elem *elem = plist->head.next;
    while (elem != &plist->tail)
    {
        if (elem == obj_elem)
        {
            return true;
        }
        elem = elem->next;
    }
    return false;
}

/*
把 plist 中每个元素 elem 和 arg 回调给 function
arg 给function 判断 elem 是否符合条件
本函数用来遍历list 所有元素，逐个判断是否有符合条件的元素，有则返回地址，无返回NULL
*/
struct list_elem *list_traversal(struct list *plist, function func, int arg)
{
    struct list_elem *elem = plist->head.next;
    if (list_empty(plist))
    {
        return NULL;
    }
    while (elem != &plist->tail)
    {
        if (func(elem, arg))
        {
            return elem;
        }
        elem = elem->next;
    }
    return NULL;
}

//链表长度
uint32_t list_len(struct list *plist)
{
    struct list_elem *elem = plist->head.next;
    uint32_t length = 0;
    while (elem != &plist->tail)
    {
        length++;
        elem = elem->next;
    }
    return length;
}

//链表是否为空
bool list_empty(struct list *plist)
{
    return (plist->head.next == &plist->tail ? true : false);
}