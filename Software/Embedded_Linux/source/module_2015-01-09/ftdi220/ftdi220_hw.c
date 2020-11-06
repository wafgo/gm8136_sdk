/* ftdi220_hw.c */
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
#include <linux/wait.h>
#include <linux/proc_fs.h>
#include <linux/kfifo.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "ftdi220.h"

//#define RESET_WORKAROUND

#ifdef VIDEOGRAPH_INC
//#include "common.h"
#include "log.h"    //include log system "printm","damnit"...
#include "video_entity.h"   //include video entity manager
#include "ftdi220_vg.h"
#endif

extern void platform_mclk_onoff(int dev_id, int on_off);
extern int use_ioremap_wc;
extern int debug_mode;
extern unsigned int  debug_value;
int yuyv = 0, allStaticEn = 0; /* auto */
unsigned int temporal_nr = 0, spatial_nr = 0;
unsigned int sec_temporal_nr = 0, sec_spatial_nr = 0;

void ftdi220_dump_register(unsigned int base)
{
	int i = 0;
	unsigned int byte_size = 0x80;
	unsigned int value = 0;
	printk("*****\n");
	for (i = 0; i < byte_size; i+=4)
	{
		value = ioread32(base + i);
		printk("@0x%08X 0x%08X\n", (base + i), value);
	}

}
/*
 * Job finish, called from ISR
 */
int ftdi220_job_finish(int dev, struct ftdi220_job *cur_job, int success)
{
    atomic_dec(&chan_info[cur_job->chan_id].list_cnt);
    atomic_inc(&chan_info[cur_job->chan_id].finish_cnt);    // increase 1 for job statistic.

    cur_job->status = success;

    if (debug_mode >= DEBUG_VG)  printk("%s \n", __func__);

    /* callback to VG */
    if (cur_job->fops.callback)  {
        cur_job->fops.callback(cur_job, cur_job->private);
        if (debug_mode >= DEBUG_ERR)
            printk("callback for chan_id = %d, jobid = %u\n", cur_job->chan_id, cur_job->jobid);
    }

    return 0;
}

void ftdi220_hw_init(int dev)
{
    u32 base = priv.engine[dev].ftdi220_reg;

    /* keep the value for proc read/write later */
    if (dev == 0) {
        /* default value */
        temporal_nr = ioread32(base + TDP);
        temporal_nr &= ~(0x1F | (0xF << 8) | (0xF << 12));
        temporal_nr |= (0x8 | (0x2 << 8) | (0x2 << 12));
        sec_temporal_nr = temporal_nr;
        sec_temporal_nr |= (0x1 << 31); /* TemporalEn */

        spatial_nr = ioread32(base + SDP);
        /* Spatial function is disabled forever due to noise cancellation not good */
        spatial_nr &= ~(0x1 << 31);
        sec_spatial_nr = spatial_nr;
    }
}


int ftdi220_trigger(int dev, ftdi220_job_t *job)
{
    unsigned int    hw_cmd = 0, command;
    volatile int    status, loop = 0;
    unsigned int    value, base = priv.engine[dev].ftdi220_reg;
    ftdi220_param_t *param = &job->param;

    /* measure the performance
     */
    if (job->fops.pre_process)
        job->fops.pre_process(job, job->private);

    if (debug_mode >= DEBUG_VG)
        printk("%s, trigger job%d \n", __func__, job->jobid);

    priv.engine[dev].handled ++;
    command = param->command;

    platform_mclk_onoff(dev, 1);

    if (loop) {}

#ifdef RESET_WORKAROUND /* workaround for booting may stuck the IP */
    iowrite32(0x2, base + INTR);
    do {
        value = ioread32(base + INTR) & (0x1 << 4);
        loop ++;

        if (loop > 0x10000) {
            printk("%s, 3DI IP reset fail! \n", __func__);
            break;
        }
    } while (value);
#endif

    iowrite32(param->ff_y_ptr, base + FF_Y_ADDR);
    iowrite32(param->fw_y_ptr, base + FW_Y_ADDR);
    iowrite32(param->cr_y_ptr, base + CR_Y_ADDR);
    iowrite32(param->nt_y_ptr, base + NT_Y_ADDR);

    if (!param->ff_y_ptr || !param->fw_y_ptr || !param->cr_y_ptr || !param->nt_y_ptr)
        panic("%s, wrong frame buffer address: 0x%x 0x%x 0x%x 0x%x \n", __func__,
            param->ff_y_ptr, param->fw_y_ptr, param->cr_y_ptr, param->nt_y_ptr);

    /* Temporal denoise */
    value = ioread32(base + TDP);
    if (command & OPT_CMD_DN_TEMPORAL) {
            value = (job->nr_stage == 2) ? sec_temporal_nr : temporal_nr;
    } else {
        value &= ~(0x1 << 31);
    }
    iowrite32(value, base + TDP);

    /* Spatial denoise */
    value = ioread32(base + SDP);
    if (command & OPT_CMD_DN_SPATIAL) {
        value = (job->nr_stage == 2) ? sec_spatial_nr : spatial_nr;
    } else {
        value &= ~(0x1 << 31);
    }
    iowrite32(value, base + SDP);

    if (debug_mode >= DEBUG_WARN) {
        printk("%s, frame_type = %d, command: 0x%x, dev: %d \n", __func__, param->frame_type,
                                param->command, dev);
    }

    if (command & OPT_CMD_DN) {
        /* TopFieldBackEn, BotFieldBackEn will be wrote out */
        hw_cmd |= ((0x1 << 31) | (0x1 << 29));
    }

    if (param->frame_type == FRAME_TYPE_2FRAMES) {
        if (command & OPT_CMD_DI) {
            /* 60i to 60p */
            iowrite32(param->di0_ptr, base + DI0_ADDR);
            iowrite32(param->di1_ptr, base + DI1_ADDR);
            /* TopDiOutEn, TopMdEn, BotDioutEn, BotMdEn */
            hw_cmd |= ((0x1 << 30) | (0x1 << 28) | (0x1 << 22) | (0x1 << 21));
        }

        if (command & OPT_CMD_OPP_COPY) {
            /* TopDiOutEn, BotDioutEn, AllStatic */
            hw_cmd |= ((0x1 << 30) | (0x1 << 28) | (0x1 << 23));
            iowrite32(param->di0_ptr, base + DI0_ADDR);
            iowrite32(param->di1_ptr, base + DI1_ADDR);
        }
    }

    if (param->frame_type == FRAME_TYPE_YUV422FRAME_60P) {
        if (command & OPT_CMD_DI) {
            if (!param->di0_ptr || !param->di1_ptr || !param->crwb_ptr || !param->ntwb_ptr)
                panic("%s, wrong frame buffer address1 => 0x%x 0x%x 0x%x 0x%x \n", __func__,
                    param->di0_ptr, param->di1_ptr, param->crwb_ptr, param->ntwb_ptr);

            /* 60i to 60p */
            iowrite32(param->di0_ptr, base + DI0_ADDR);
            iowrite32(param->di1_ptr, base + DI1_ADDR);
            /* TopDiOutEn, TopMdEn, BotDioutEn, BotMdEn */
            hw_cmd |= ((0x1 << 30) | (0x1 << 28) | (0x1 << 22) | (0x1 << 21));

            /* CR_WB_AddrEn, NT_WB_AddrEn */
            hw_cmd |= (0x3 << 18);
            /* CRWB addr, BTWB addr */
            iowrite32(param->crwb_ptr, base + CRWB_ADDR);
            iowrite32(param->ntwb_ptr, base + NTWB_ADDR);
        }

        if (command & OPT_CMD_OPP_COPY) {
            if (!param->di0_ptr || !param->di1_ptr || !param->crwb_ptr || !param->ntwb_ptr)
                panic("%s, wrong frame buffer address2 => 0x%x 0x%x 0x%x 0x%x \n", __func__,
                    param->di0_ptr, param->di1_ptr, param->crwb_ptr, param->ntwb_ptr);

            /* TopDiOutEn, BotDioutEn, AllStatic */
            hw_cmd |= ((0x1 << 30) | (0x1 << 28) | (0x1 << 23));
            iowrite32(param->di0_ptr, base + DI0_ADDR);
            iowrite32(param->di1_ptr, base + DI1_ADDR);

            /* CR_WB_AddrEn, NT_WB_AddrEn */
            hw_cmd |= (0x3 << 18);
            /* CRWB addr, BTWB addr */
            iowrite32(param->crwb_ptr, base + CRWB_ADDR);
            iowrite32(param->ntwb_ptr, base + NTWB_ADDR);
        }
    }

    /* denoise only */
    if (param->frame_type == FRAME_TYPE_2FIELDS) {
        if (command & OPT_CMD_DN) {
            /* Field mode is field mode (line by line) */
            hw_cmd |= (0x1 << 25);
            if (command & (OPT_WRITE_NEWBUF | OPT_HYBRID_WRITE_NEWBUF)) {
                /* CR_WB_AddrEn, NT_WB_AddrEn */
                hw_cmd |= (0x3 << 18);
                /* CRWB addr, BTWB addr */
                iowrite32(param->crwb_ptr, base + CRWB_ADDR);
                iowrite32(param->ntwb_ptr, base + NTWB_ADDR);
            }
        } else {
            /* frame copy.
             * TopDiOutEn, BotDioutEn, AllStatic,
             */
            hw_cmd |= ((0x1 << 30) | (0x1 << 28) | (0x1 << 23));
            iowrite32(param->di0_ptr, base + DI0_ADDR);
            iowrite32(param->di1_ptr, base + DI1_ADDR);
        }
    }

    if (param->frame_type == FRAME_TYPE_YUV422FRAME) {
        if (command & OPT_CMD_DI) {
            /* CR_WB_AddrEn, NT_WB_AddrEn */
            hw_cmd |= (0x3 << 18);
            /* CRWB addr, BTWB addr */
            iowrite32(param->crwb_ptr, base + CRWB_ADDR);
            iowrite32(param->ntwb_ptr, base + NTWB_ADDR);
            /* TopDiOutEn,  */
            hw_cmd |= ((0x1 << 30));
            hw_cmd |= ((0x3 << 21)); /* TopMdEn, BotMdEn */
            iowrite32(param->di0_ptr, base + DI0_ADDR);
        } else if (command & OPT_CMD_DN) {
            /* denoise only
             */
            if (command & (OPT_WRITE_NEWBUF | OPT_HYBRID_WRITE_NEWBUF)) {
                /* CR_WB_AddrEn, NT_WB_AddrEn */
                hw_cmd |= (0x3 << 18);
                iowrite32(param->crwb_ptr, base + CRWB_ADDR);
                iowrite32(param->ntwb_ptr, base + NTWB_ADDR);
            }
        } else {
            /* frame copy.
             * TopDiOutEn, BotDioutEn, AllStatic
             */
            hw_cmd |= ((0x1 << 30) | (0x1 << 28) | (0x1 << 23));
            iowrite32(param->di0_ptr, base + DI0_ADDR);
            iowrite32(param->di1_ptr, base + DI1_ADDR);
        }
    }

    /* Limited denoised/de-interlaced frame height/width register */
    if ((param->width == param->limit_width) && (param->height == param->limit_height))
        iowrite32(0x0, base + OFHW);
    else
        iowrite32((param->width << 16) | param->height, base + OFHW);

    iowrite32((param->limit_width << 16) | param->limit_height, base + LFHW);
    iowrite32((param->offset_x << 16) | param->offset_y, base + SXY);

    iowrite32(0x1, base + INTR);    //clear frame done status
    status = ioread32(base + INTR);
    if (status & 0x1) {
        printk("Fail!Status should be clear to 0x0, but 0x%x\n", status);
        return 0;
    }

    //measure function
    if (job->fops.mark_engine_start)
        job->fops.mark_engine_start(job, job->private);

#if 1 /* Ablert ask for all static */
    if (allStaticEn == 1)
        hw_cmd |= (0x1 << 23);
    /* 1 for input data's format is YUYV */
    if (yuyv == 1)
        hw_cmd |= (0x1 << 24);
#endif

    //platform_mclk_onoff(dev, 1);
    //fire
    iowrite32(hw_cmd | 0x1, base + COMD);

    /* drain write buffer */
    if (use_ioremap_wc)
        ioread32(base + COMD);

    if (0) {
        int i;

        printk("job%d register dump===> ", job->jobid);
        for (i = 0x0; i <= 0x30; i += 0x4) {
            if (!(i % 0x10))
                printk("\n");
            printk("reg0x%x=0x%x, ", i, ioread32(base + i));
        }
        printk("reg0x6c=0x%x, reg0x70=0x%x, ", ioread32(base + 0x6c), ioread32(base + 0x70));
        printk("\n\n");
    }

    return 0;
}

/* this function is executed in atomic environment
 */
ftdi220_job_t* ftdi220_get_node_to_fire(int dev)
{

	int		len = 0;
	int 	fire = 0;
	ftdi220_job_t   *node, *ne, *cur_job = NULL;

    len = atomic_read(&priv.engine[dev].list_cnt);

    if (len == 0) /* means ISR might wakeup and get the job already */
        goto exit;

	list_for_each_entry_safe(node, ne, &priv.engine[dev].list, list) {
        list_del(&node->list);
        atomic_dec(&priv.engine[dev].list_cnt);
        memcpy(&priv.engine[dev].cur_job, node, sizeof(ftdi220_job_t));
#ifdef USE_JOB_MEMORY
        ftdi220_drv_dealloc_jobmem(node);
#else
        kmem_cache_free(priv.job_cache, node);
#endif
		priv.eng_node_count--;
        cur_job = &priv.engine[dev].cur_job;

         /* when ftdi220_stop_channel() function is called, only update the core status */
        if (cur_job->status & (JOB_STS_FLUSH | JOB_STS_DONOTHING)) {
            /* mark this job is finished */
            cur_job->hardware = 0;
            if (cur_job->fops.mark_engine_finish)
                cur_job->fops.mark_engine_finish(cur_job, cur_job->private);

            if (cur_job->status & JOB_STS_FLUSH)
                ftdi220_job_finish(dev, cur_job, JOB_STS_FLUSH);
            else
                ftdi220_job_finish(dev, cur_job, JOB_STS_DONE);

            continue;
        }
		fire = 1;
        cur_job->status = JOB_STS_ONGOING;
        break;
    }

	exit:
	if (fire == 0) {
		cur_job = NULL;
	}
	return cur_job;
}

int ftdi220_start_and_wait(ftdi220_job_t *job)
{
    int dev = job->dev;
    ftdi220_job_t *cur_job = NULL;
    unsigned long flags;

    DRV_LOCK(flags);

    if (!ENGINE_BUSY(job->dev)) {
        cur_job = ftdi220_get_node_to_fire(dev);
        if (cur_job) {
#ifdef VIDEOGRAPH_INC
            job_node_t  *node_item = (job_node_t *)cur_job->private;
            struct video_entity_job_t   *vjob = (struct video_entity_job_t *)node_item->private;

            if (debug_value)
                printm("DI", "start_and_wait got job to fire, dev: %d, status: 0x%x, jid: %d \n", dev, node_item->status, vjob->id);
#endif
            LOCK_ENGINE(dev);
            ftdi220_trigger(dev, cur_job);
        }
    }

    DRV_UNLOCK(flags);
    return 0;
}

irqreturn_t ftdi220_interrupt(int irq, void *devid)
{
    int          dev = (int)devid;
    unsigned int base, status;
    ftdi220_job_t *cur_job = NULL;

    platform_mclk_onoff(dev, 0);
    base = priv.engine[dev].ftdi220_reg;

    status = ioread32(base + INTR);
    iowrite32(1, base + INTR);
    if (use_ioremap_wc)
        ioread32(base + INTR);

    if (!ENGINE_BUSY(dev))
        printk("%s, bug, dev=%d! \n", __func__,  dev);

    if ((status & 0x1) == 0) {
        panic("%s, hw bug: 0x%x! \n", __func__,  status);
    }

    if(debug_mode >= DEBUG_ERR)
        printk("%s->: Engine %d IRQ %d base 0x%x\n",__FUNCTION__,(int)dev,irq,base);

    cur_job = &priv.engine[dev].cur_job;

    if (debug_mode >= DEBUG_VG)
        printk("%s(dev:%d), job%d is done.\n", __func__, dev, cur_job->jobid);

    /* mark this job is finished */
    cur_job->hardware = 1;
    if (cur_job->fops.mark_engine_finish)
        cur_job->fops.mark_engine_finish(cur_job, cur_job->private);

    /* measure the performance */
    if (cur_job->fops.post_process)
        cur_job->fops.post_process(cur_job, cur_job->private);

    /* callback the job */
	ftdi220_job_finish(dev, cur_job, JOB_STS_DONE);

	cur_job = ftdi220_get_node_to_fire(dev);
    if (cur_job) {
#ifdef VIDEOGRAPH_INC
        job_node_t  *node_item = (job_node_t *)cur_job->private;
        struct video_entity_job_t   *job = (struct video_entity_job_t *)node_item->private;

        if (debug_value)
            printm("DI", "intr got job to fire. dev: %d, status: 0x%x, jid: %d\n", dev, node_item->status, job->id);
#endif
        ftdi220_trigger(dev, cur_job);
    } else {
        memset(&priv.engine[dev].cur_job, 0, sizeof(priv.engine[dev].cur_job));
        UNLOCK_ENGINE(dev);
        if (debug_mode >= DEBUG_VG)
            printk("%s, dev:%d unlock engine\n", __func__, dev);
    }

    if (debug_mode >= DEBUG_ERR)
        printk("%s<- Engine %d\n",__FUNCTION__,(int)dev);

    /* check again */
    if (!list_empty(&priv.engine[dev].list) && !ENGINE_BUSY(dev))
        panic("%s, bug1! \n", __func__);

    return IRQ_HANDLED;
}

