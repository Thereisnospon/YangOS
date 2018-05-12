#include "sync.h"
#include "list.h"
#include "global.h"
#include "debug.h"
#include "interrupt.h"

//初始化信号量
void sema_init(struct semaphore *psema, uint8_t value)
{
    psema->value = value;       //信号量赋初始值
    list_init(&psema->waiters); //初始化信号量等待队列
}
//初始化锁
void lock_init(struct lock *plock)
{
    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    sema_init(&plock->semaphore, 1); //信号量初始值为1
}
//信号量down操作
void sema_down(struct semaphore *psema)
{
    enum intr_status old_status = intr_disable();
    //被唤醒时，可能锁又被其他线程获取，要循环判断
    while (psema->value == 0) //如果为 0 ，表示已经被别人占有
    {
        ASSERT(!elem_find(&psema->waiters, &running_thread()->general_tag));
        //当前线程不应该在信号的等待队列中
        if (elem_find(&psema->waiters, &running_thread()->general_tag))
        {
            PANIC("sema down: thread blocked has been in waiter_list");
        }
        //当前线程加入到等待队列中，并阻塞自己
        list_append(&psema->waiters, &running_thread()->general_tag);
        thread_block(TASK_BLOCKED); // 切换cpu。 恢复时从这里继续执行
    }
    //若 value 为1 被唤醒后，会执行下面的代码，也就是获得了锁
    psema->value--;
    ASSERT(psema->value == 0);
    //恢复之前的中断状态
    intr_set_status(old_status);
}

//信号量up
void sema_up(struct semaphore *psema)
{
    //关中断，保证原子操作
    enum intr_status old_status = intr_disable();
    ASSERT(psema->value == 0);
    if (!list_empty(&psema->waiters))
    {
        struct task_struct *thread_blocked = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));
        thread_unblock(thread_blocked);
    }
    psema->value++;
    ASSERT(psema->value == 1);
    intr_set_status(old_status);
}

//获取锁
void lock_acquire(struct lock *plock)
{
    if (plock->holder != running_thread())
    {
        sema_down(&plock->semaphore);
        plock->holder = running_thread();
        ASSERT(plock->holder_repeat_nr == 0);
        plock->holder_repeat_nr = 1;
    }
    else
    {
        plock->holder_repeat_nr++;
    }
}
void lock_release(struct lock *plock)
{
    ASSERT(plock->holder == running_thread());
    if (plock->holder_repeat_nr > 1)
    {
        plock->holder_repeat_nr--;
        return;
    }
    ASSERT(plock->holder_repeat_nr == 1);
    plock->holder = NULL; //把锁的持有者置为空放在 up 操前
    plock->holder_repeat_nr = 0;
    sema_up(&plock->semaphore);//v(up)操作
}