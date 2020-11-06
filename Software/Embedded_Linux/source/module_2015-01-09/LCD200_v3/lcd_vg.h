#ifndef __LCD_VG_H__
#define __LCD_VG_H__

#define USE_KTRHEAD

#define FIFO_DEPTH   20

struct _lcd_priv {
    spinlock_t fifo_lock;
    struct kfifo job_queue;
    spinlock_t pfifo_lock;      //purge fifo lock
    struct kfifo purge_queue;
    spinlock_t cfifo_lock;      //callback fifo lock
    struct kfifo cb_queue;
    struct workqueue_struct *workq;
    spinlock_t irqlock;
};

#define DBG_PUT_JOB         (0x1 << 0)
#define DBG_JOB_CALLBACK    (0x1 << 1)
#define DBG_PURGE_JOB       (0x1 << 2)
#define DBG_JOB_FLUSH       (0x1 << 3)
#define DBG_FLUSH_PURGE_JOB (0x1 << 4)
#define DBG_FRAME_RATE      (0x1 << 5)
#endif
