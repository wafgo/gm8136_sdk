/* ftdi220_drv.c */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/hdreg.h>	/* HDIO_GETGEO */
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/miscdevice.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/kfifo.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/semaphore.h>
#include <linux/platform_device.h>
#include "ftdi220.h"
#include "ftdi220_prm.h"

#ifdef VIDEOGRAPH_INC
//#include "common.h"
#include "log.h"    //include log system "printm","damnit"...
#include "video_entity.h"   //include video entity manager
#include "ftdi220_vg.h"
#endif

#include "frammap_if.h"

ushort   pwm = 0;
module_param(pwm, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(pwm, "pwm");

int use_ioremap_wc = 0;
module_param(use_ioremap_wc, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(use_ioremap_wc, "1 for use ncb");

int minor_balance = 0;
module_param(minor_balance, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(minor_balance, "1 for use minor as balance");

int dbg_enhance_3dnr = 0;
int engine_mask = 0x0;

/* Global declaration
 */
extern unsigned int log_level;

ftdi220_priv_t priv;
chan_info_t  chan_info[MAX_CHAN_NUM];

int debug_mode=DEBUG_QUIET;
//int debug_mode=DEBUG_WARN;

/*
 * Local Variables Declaration
 */

/* for multiple job. The upper layer may call ftdi220_put_job by disalbing interrupt.
 * thus we can't allocate memory in interrupt context
 */
typedef struct {
    ftdi220_job_t   job;
    int             occupy;
} job_memory_t;

job_memory_t  *job_memory = NULL;

#define JOB_MEMORY_CNT  (MAX_CHAN_NUM * 3)

/*
 * Local Functions Declaration
 */
int sanity_check(ftdi220_param_t *param);
void (*printeng_vg)(char *page, int *len, void *data) = NULL;

/*
 * Extern function declarations
 */
extern irqreturn_t ftdi220_interrupt(int irq, void *devid);
extern int platform_init(void);
extern int platform_exit(void);
extern int is_print(int engine,int minor);

/* the return memory is ftdi220_job_t
 */
ftdi220_job_t *ftdi220_drv_alloc_jobmem(void)
{
    int i;
    job_memory_t *ptr = job_memory;
    unsigned long flags;

    spin_lock_irqsave(&priv.lock, flags);

    for (i = 0; i < JOB_MEMORY_CNT; i ++) {
        if (ptr->occupy == 0) {
            ptr->occupy = 1;
            break;
        }
        ptr ++;
    }

    spin_unlock_irqrestore(&priv.lock, flags);

    if (i < JOB_MEMORY_CNT)
        return (void *)&ptr->job;

    panic("%s, no memory! \n", __func__);

    return NULL;
}

/* The parameter is ftdi220_job_t
 */
void ftdi220_drv_dealloc_jobmem(ftdi220_job_t *job_mem)
{
    unsigned long flags;
    job_memory_t *ptr = container_of(job_mem, job_memory_t, job);

    spin_lock_irqsave(&priv.lock, flags);
    ptr->occupy = 0;
    spin_unlock_irqrestore(&priv.lock, flags);
}

/*
 * Create engine and FIFO
 */
int add_engine(int dev, unsigned long addr, unsigned long irq)
{
    int idx, ret = 0;

    idx = priv.eng_num;

    priv.engine[idx].dev = idx;
    priv.engine[idx].ftdi220_reg = addr;
    priv.engine[idx].irq = irq;
    INIT_LIST_HEAD(&priv.engine[idx].list);
    priv.engine[idx].enhance_3dnr = dbg_enhance_3dnr;

    priv.eng_num ++;
    switch (priv.eng_num) {
      case 1:
        engine_mask = 0;
        break;
      case 2:
        engine_mask = 1;
        break;
      default:
        panic("3di: must modify the code! priv.eng_num = %d \n", priv.eng_num);
        break;
    }

    return ret;
}


/* Note: ftdi220_init_one() will be run before ftdi220_drv_init() */
static int ftdi220_init_one(struct platform_device *pdev)
{
	struct resource *mem, *irq;
	unsigned long   vaddr;
	int    ret;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		printk(KERN_DEBUG "no mem resource?\n");
		return -ENODEV;
	}

	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!irq) {
		printk(KERN_DEBUG "no irq resource?\n");
		return -ENODEV;
	}

    if (use_ioremap_wc)
        vaddr = (unsigned long)ioremap_wc(mem->start, PAGE_ALIGN(mem->end - mem->start));
    else
        vaddr = (unsigned long)ioremap_nocache(mem->start, PAGE_ALIGN(mem->end - mem->start));

    if (unlikely(!vaddr))
        panic("%s, no virtual address! \n", __func__);

	ret = add_engine(pdev->id, vaddr, irq->start);
    if (!ret) {
	    /* keep the private data */
		set_default_param(priv.eng_num - 1);
        platform_set_drvdata(pdev, &priv.engine[priv.eng_num - 1]);
	}
	/* hw init */
    ftdi220_hw_init(priv.eng_num - 1);

	return ret;
}


static int ftdi220_remove_one(struct platform_device *pdev)
{
    platform_set_drvdata(pdev, NULL);

    return 0;
}


static struct platform_driver ftdi220_driver = {
	.probe	= ftdi220_init_one,
	.remove	= ftdi220_remove_one,
	.driver = {
	        .name = "FTDI220",
	        .owner = THIS_MODULE,
	    },
};

/* Choose a engine ID for job service
 */
int choose_engineid(struct ftdi220_job *job)
{
    int     chnum = job->chan_id;
    int     list_cnt, dev, idx = -1;
    u32     max_cnt, finish_cnt;

    if (minor_balance) {
        /* balance by minor */
        return job->param.minor & engine_mask;
    }

    /* minor_balance = 0 means normal balance by chan_id */
    max_cnt = finish_cnt = (u32)-1;

    /* Because the job has the dependency */
    list_cnt = atomic_read(&chan_info[chnum].list_cnt);

    if (list_cnt) {
        idx = chan_info[chnum].dev;
        return idx;
    }

    /* need to choose new engine to service
     */
    for (dev = 0; dev < priv.eng_num; dev++) {
        if (ENGINE_BUSY(dev))
            continue;
        /* this engine is available */
        idx = dev;
        break;
    }

    if (idx < 0) {
        /* choose an engine who has less loading to service
         */
        for (dev = 0; dev < priv.eng_num; dev++) {
            /* the compare rules are:
             * (1) less qlen is prefered
             * (2) less job finish count is prefered
             */
            list_cnt = atomic_read(&priv.engine[dev].list_cnt);
            if (list_cnt < max_cnt) {
                max_cnt = list_cnt;
                finish_cnt = priv.engine[dev].handled;
                idx = dev;
            }
            else if ((list_cnt == max_cnt) && (finish_cnt > priv.engine[dev].handled)) {
                finish_cnt = priv.engine[dev].handled;
                idx = dev;
            }
        }
    }

    if (idx < 0)
        panic("FTDI220: Can't find a proper engine to serivce, job skipped! \n");

    chan_info[chnum].dev = idx;

    return idx;
}

/* return 0 for success or negative value for fail
 */
int ftdi220_job_add(ftdi220_job_t *job)
{
    unsigned long   flags;
    ftdi220_job_t   *node;
    int             dev;

    /* increase the job sequence id */
    priv.jobseq = (priv.jobseq + 1) & 0xFFFFFF;
    job->jobid = priv.jobseq;

#ifdef USE_JOB_MEMORY
    node = (ftdi220_job_t *)ftdi220_drv_alloc_jobmem();
#else
    if (in_interrupt() || in_atomic())
        node = (ftdi220_job_t *)kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
    else
        node = (ftdi220_job_t *)kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
#endif

	priv.eng_node_count++;

    if (unlikely(!node))
        panic("%s, No memory for node! \n", __func__);

    dev = choose_engineid(job);
    job->dev = dev;
    memcpy(node, job, sizeof(ftdi220_job_t));
    INIT_LIST_HEAD(&node->list);
    node->nr_stage = 1;

    DRV_LOCK(flags);

    /* increase 1 for job statistic. */
    atomic_inc(&chan_info[job->chan_id].list_cnt);
    /* chained to the specific engine */
    list_add_tail(&node->list, &priv.engine[dev].list);
    /* increase 1 for job statistic. */
    atomic_inc(&priv.engine[dev].list_cnt);
    DRV_UNLOCK(flags);

	return 0;
}

int ftdi220_2jobs_add(ftdi220_job_t *job)
{
    unsigned long   flags;
    ftdi220_job_t   *node, *sec_node;
    int dev, line_ofs;

    /* increase the job sequence id */
    priv.jobseq = (priv.jobseq + 1) & 0xFFFFFF;
    job->jobid = priv.jobseq;

#ifdef USE_JOB_MEMORY
    node = (ftdi220_job_t *)ftdi220_drv_alloc_jobmem();
#else
    if (in_interrupt() || in_atomic())
        node = (ftdi220_job_t *)kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
    else
        node = (ftdi220_job_t *)kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
#endif
    priv.eng_node_count ++;
    if (unlikely(!node))
        panic("%s, No memory for node! \n", __func__);

    dev = choose_engineid(job);
    job->dev = dev;
    memcpy(node, job, sizeof(ftdi220_job_t));
    INIT_LIST_HEAD(&node->list);

    node->fops.post_process = NULL;
    node->fops.mark_engine_finish = NULL;
    node->fops.callback = NULL;
    node->nr_stage = 1; /* first pass */

    /*
     * start to construct the second node
     */
#ifdef USE_JOB_MEMORY
    sec_node = (ftdi220_job_t *)ftdi220_drv_alloc_jobmem();
#else
    if (in_interrupt() || in_atomic())
        sec_node = (ftdi220_job_t *)kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
    else
        sec_node = (ftdi220_job_t *)kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
#endif
    priv.eng_node_count ++;
    if (unlikely(!sec_node))
        panic("%s, No memory for sec_node! \n", __func__);
    memcpy(sec_node, job, sizeof(ftdi220_job_t));
    INIT_LIST_HEAD(&sec_node->list);

    sec_node->fops.pre_process = NULL;
    sec_node->fops.mark_engine_start = NULL;
    sec_node->nr_stage = 2; /* 2nd pass */

    /* orginal:
     *  frame(c) + frame(ref) --> frame(c)
     * new:
     *  1st pass: frame(c) + frame(ref) --> frame(temp)
     *  2nd pass: frame(c) + frame(temp) --> frame(c)
     */
    line_ofs = job->param.width << 1;    //yuv422
#if 0
    if (job->param.command & (OPT_WRITE_NEWBUF | OPT_HYBRID_WRITE_NEWBUF)) {
        /* 1st pass: the result is to temp buffer */
        node->param.crwb_ptr = priv.engine[dev].extrabuf_paddr;
        node->param.ntwb_ptr = priv.engine[dev].extrabuf_paddr + line_ofs;
        /* 2nd pass: the reference buffer is temp buffer */
        sec_node->param.ff_y_ptr = priv.engine[dev].extrabuf_paddr;
        sec_node->param.fw_y_ptr = priv.engine[dev].extrabuf_paddr + line_ofs;
        panic("3di: not support this mode for enhance job! \n");
    } else {
        /* 1st pass: the result is to temp buffer */
        node->param.crwb_ptr = priv.engine[dev].extrabuf_paddr;
        node->param.ntwb_ptr = priv.engine[dev].extrabuf_paddr + line_ofs;
        /* 2nd pass: the reference buffer is temp buffer */
        sec_node->param.ff_y_ptr = priv.engine[dev].extrabuf_paddr;
        sec_node->param.fw_y_ptr = priv.engine[dev].extrabuf_paddr + line_ofs;
    }
#endif
    priv.engine[dev].enhance_nr_cnt ++;

    DRV_LOCK(flags);

    /* increase 1 for job statistic. */
    atomic_inc(&chan_info[job->chan_id].list_cnt);
    /* chained to the specific engine */
    list_add_tail(&node->list, &priv.engine[dev].list);
    /* increase 1 for job statistic. */
    atomic_inc(&priv.engine[dev].list_cnt);

    /* increase 1 for job statistic. */
    atomic_inc(&chan_info[job->chan_id].list_cnt);
    /* chained to the specific engine */
    list_add_tail(&sec_node->list, &priv.engine[dev].list);
    /* increase 1 for job statistic. */
    atomic_inc(&priv.engine[dev].list_cnt);

    DRV_UNLOCK(flags);

    return 0;
}

/*
 * Put a job to fti220 module core
 */
int ftdi220_put_job(int chan_id, ftdi220_param_t *param, struct f_ops *fops, void *private, job_status_t status)
{
    ftdi220_job_t   job;
    int ret;

    memset(&job, 0, sizeof(ftdi220_job_t));

    if (chan_id >= MAX_CHAN_NUM) {
        printk("3DI: wrong channel number1 %d \n", chan_id);
        return -1;
    }

    if (param->chan_id != chan_id) {
        printk("3DI: wrong channel number2 %d \n", param->chan_id);
        return -1;
    }

    if (sanity_check(param) < 0)
        return -1;

    memcpy(&job.param, param, sizeof(ftdi220_param_t));
    job.chan_id = chan_id;
    memcpy(&job.fops, fops, sizeof(struct f_ops));
    job.private = private;
    job.status = status;

    /* add a new job and also trigger hw */
    if (dbg_enhance_3dnr && (param->frame_type == FRAME_TYPE_YUV422FRAME || param->frame_type == FRAME_TYPE_YUV422FRAME_60P) &&
                                            ((param->command & OPT_CMD_DIDN) == OPT_CMD_DN))
        ret = ftdi220_2jobs_add(&job);
    else
        ret = ftdi220_job_add(&job);
    if (debug_mode >= DEBUG_VG)
        printk("%s, put jobid = %d \n", __func__, job.jobid);

    if (likely(!ret))
        ftdi220_start_and_wait(&job);

    return ret;
}

/* do basic parameters check */
int sanity_check(ftdi220_param_t *param)
{
    int ret = 0;

    if (param->chan_id < 0 || param->chan_id >= MAX_CHAN_NUM) {
        printk("Error channel number %d, max is %d \n", param->chan_id, MAX_CHAN_NUM);
        ret = -EINVAL;
    }

    if (param->command == OPT_CMD_DISABLED)
        return 0;

    /* No matter which case, ff, fw, cr, nt should not zero */
    if (!param->width || !param->height || !param->ff_y_ptr || !param->fw_y_ptr || !param->cr_y_ptr ||
        !param->nt_y_ptr) {
        printk("%s, wrong value! \n", __func__);
        ret = -EINVAL;
        goto exit;
    }

    if ((param->frame_type == FRAME_TYPE_YUV422FRAME_60P) && (param->command & OPT_CMD_DI)) {
        if (!param->di0_ptr || !param->di1_ptr) {
            printk("%s, wrong value11! \n", __func__);
            ret = -EINVAL;
            goto exit;
        }

        if (!param->crwb_ptr || !param->ntwb_ptr) {
            printk("%s, wrong value11-1! \n", __func__);
            ret = -EINVAL;
            goto exit;
        }
    }

    if (param->frame_type == FRAME_TYPE_2FRAMES) {
        if (!param->di0_ptr || !param->di1_ptr) {
            printk("%s, wrong value1! \n", __func__);
            ret = -EINVAL;
            goto exit;
        }
    }

    if ((param->frame_type == FRAME_TYPE_YUV422FRAME) && (param->command & OPT_CMD_DI)) {
        if (!param->di0_ptr) {
            printk("%s, wrong value2! \n", __func__);
            ret = -EINVAL;
            goto exit;
        }
    }

    if (param->frame_type & (FRAME_TYPE_2FIELDS | FRAME_TYPE_YUV422FRAME)) {
        /* command is denoise or deinterlace */
        if (!(param->command & (OPT_CMD_FRAME_COPY | OPT_CMD_DISABLED)) && (param->command & (OPT_WRITE_NEWBUF | OPT_HYBRID_WRITE_NEWBUF))) {
            if (!param->crwb_ptr || !param->ntwb_ptr) {
                printk("%s, wrong value3! \n", __func__);
                ret = -EINVAL;
                goto exit;
            }
        }

        if ((param->frame_type == FRAME_TYPE_2FIELDS) && (param->command & OPT_CMD_DI)) {
            /* two fields only support denoise */
            printk("%s, wrong value4! \n", __func__);
            ret = -EINVAL;
            goto exit;
        }
    }

    /* exclusive test */
    if (param->command & (OPT_CMD_OPP_COPY | OPT_CMD_FRAME_COPY)) {
        if (param->command & OPT_CMD_DI) {
            printk("%s, wrong value5! \n", __func__);
            ret = -EINVAL;
            goto exit;
        }
    }

    if (param->width % WIDTH_MULT) {
        printk("FAIL: width %d must be multiple of %d\n",param->width, WIDTH_MULT);
        ret = -EINVAL;
    }

    if (param->height % HEIGHT_MULT) {
        printk("FAIL: height %d must be multiple of %d\n",param->height, HEIGHT_MULT);
        ret = -EINVAL;
    }

    if (param->limit_width + param->offset_x > param->width) {
        printk("%s, invalid width: %d, limit_width: %d, offset: %d \n", __func__,
            param->width, param->limit_width, param->offset_x);
        return -EINVAL;
    }

    if (param->limit_height + param->offset_y > param->height) {
        printk("%s, invalid height: %d, limit_height: %d, offset: %d \n", __func__,
            param->height, param->limit_height, param->offset_y);
        return -EINVAL;
    }

exit:
    if (debug_mode >= DEBUG_DUMPALL || ret < 0) {
        printk("(%dx%d)==>\n",param->width, param->height);
        printk("    ff_y_ptr=0x%x\n",(unsigned int)param->ff_y_ptr);
        printk("    fw_y_ptr=0x%x\n",(unsigned int)param->fw_y_ptr);
        printk("    cr_y_ptr=0x%x\n",(unsigned int)param->cr_y_ptr);
        printk("    nt_y_ptr=0x%x\n",(unsigned int)param->nt_y_ptr);
        printk("    di0_ptr=0x%x\n",(unsigned int)param->di0_ptr);
        printk("    di1_ptr=0x%x\n",(unsigned int)param->di1_ptr);
        printk("    crwb_ptr=0x%x\n",(unsigned int)param->crwb_ptr);
        printk("    ntwb_ptr=0x%x\n",(unsigned int)param->ntwb_ptr);
        printk("    command: 0x%x \n", param->command);
    }

    return ret;
}

#ifdef VIDEOGRAPH_INC
/* stop a specific channel */
void ftdi220_stop_channel(int chan_id)
{
    int dev;
    unsigned long flags;
    ftdi220_job_t *node = NULL;
    ftdi220_job_t *dummy_node = NULL;
#if 1
    job_node_t    *vg_node = NULL;
    struct video_entity_job_t *job = NULL;
    int engine = 0;
    int minor = 0;
#endif

    DRV_LOCK(flags);
    /* go through for each engine */
    for (dev = 0; dev < priv.eng_num; dev ++) {
        list_for_each_entry_safe(node, dummy_node, &priv.engine[dev].list, list) {
            if ((node->chan_id == chan_id) && (node->status & JOB_STS_QUEUE)) {
                node->status |= JOB_STS_FLUSH;
#if 1
                vg_node = (job_node_t *)node->private;
				job = (struct video_entity_job_t *)vg_node->private; /* nish add 20130116 */

                engine = ENTITY_FD_ENGINE(job->fd);
                minor = ENTITY_FD_MINOR(job->fd);
                DEBUG_M(LOG_DEBUG, engine, minor,"ftdi220_stop_channel\n");
#endif
			}
        }
    }
    DRV_UNLOCK(flags);
    return;
}
#endif

#include "proc.c"

static int __init ftdi220_drv_init(void)
{
    int i, dev;
    const char *devname[FTDI220_MAX_NUM]={"ftdi220-0","ftdi220-1"};;

    /* init the clock and add the device .... */
    platform_init();

    memset(&priv,0,sizeof(priv));
    memset(&chan_info[0], 0x0, sizeof(chan_info_t) * MAX_CHAN_NUM);
    for (i = 0; i < MAX_CHAN_NUM; i ++)
        chan_info[i].chan_id = i;

	/* Register the driver */
	if (platform_driver_register(&ftdi220_driver)) {
		printk("Failed to register FTDI220 driver\n");
		return -ENODEV;
	}

    //irq 41(91400000),42(91500000)
    for (dev = 0; dev < priv.eng_num; dev++) {
        if (request_irq(priv.engine[dev].irq, &ftdi220_interrupt, 0, devname[dev], (void *)dev) < 0)
            printk("Unable to allocate IRQ \n");
    }

	spin_lock_init(&priv.lock);

#ifdef USE_JOB_MEMORY
    job_memory = kmalloc(JOB_MEMORY_CNT * sizeof(job_memory_t), GFP_KERNEL);
    if (job_memory == NULL)
        panic("%s, allocate memory %d bytes fail! \n", __func__, JOB_MEMORY_CNT * sizeof(job_memory_t));
    /* clear memory */
    memset((void *)job_memory, 0, JOB_MEMORY_CNT * sizeof(job_memory_t));
#else
	priv.job_cache = kmem_cache_create("di220", sizeof(ftdi220_job_t), 0, 0, NULL);
    if (priv.job_cache == NULL)
        panic("%s, no memory for job_cache! \n", __func__);
#endif
#ifdef VIDEOGRAPH_INC
    ftdi220_vg_init();
#endif

	/* init the proc.
	 */
	proc_init();

	printk("FTDI220 Driver v%d.%d (%d engine(s)) \n", FTDI220_VER_MAJOR, FTDI220_VER_MINOR, priv.eng_num);

    return 0;
}

static void __exit ftdi220_drv_clearnup(void)
{
    int dev;

    proc_remove();

    platform_driver_unregister(&ftdi220_driver);

#ifdef USE_JOB_MEMORY
    kfree(job_memory);
    job_memory = NULL;
#else
    /* destroy the memory cache */
    kmem_cache_destroy(priv.job_cache);
#endif

#ifdef VIDEOGRAPH_INC
    ftdi220_vg_driver_clearnup();
#endif
    platform_exit();

    for (dev = 0; dev < priv.eng_num; dev++) {
        free_irq(priv.engine[dev].irq, (void *)dev);
        __iounmap((void *)priv.engine[dev].ftdi220_reg);
    }
}

void ftdi220_set_dbgfn(void (*function)(char *page, int *len, void *data))
{
    printeng_vg = function;
}

EXPORT_SYMBOL(ftdi220_put_job);

module_init(ftdi220_drv_init);
module_exit(ftdi220_drv_clearnup);
MODULE_LICENSE("GPL");

