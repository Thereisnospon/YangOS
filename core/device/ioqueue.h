#ifndef __DEVICE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H

#include "stdint.h"
#include "thread.h"
#include "sync.h"

#define bufsize 64

struct ioqueue{
    struct lock lock;
    //生产者，缓冲区不满时，继续从里面放数据，否则休眠，这里记录哪个生产者在缓冲区休眠
    struct task_struct * producer;
    //消费者，缓冲区不空时，继续从里面拿数据，否则休眠，这里记录哪个消费者在缓冲区休眠
    struct task_struct * consumer;
    char buf[bufsize]; //缓冲区大小
    int32_t head;   //队首
    int32_t tail;   //队尾
};
void ioqueue_init(struct ioqueue *ioq);
bool ioq_full(struct ioqueue *ioq);
char ioq_getchar(struct ioqueue *ioq);
void ioq_putchar(struct ioqueue *ioq, char byte);

#endif 