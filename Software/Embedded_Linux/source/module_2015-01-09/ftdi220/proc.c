#include <linux/proc_fs.h>
#include "ftdi220_prm.h"


static int proc_read_debug(char *page, char **start, off_t off, int count,int *eof, void *data);
static int proc_write_debug(struct file *file, const char *buffer, unsigned long count, void *data);
static int proc_read_enhanceNR(char *page, char **start, off_t off, int count,int *eof, void *data);
static int proc_write_enhanceNR(struct file *file, const char *buffer, unsigned long count, void *data);

extern int enhance_3dnr;
extern int dbg_enhance_3dnr;

prm_proc_t g_prm_proc[] =
{
	/* name, 			parent, entry, 	write, 						read */
	{"ftdi220",			NULL, 	NULL, 	NULL,						NULL},
	{"debug_proc",		NULL, 	NULL, 	proc_write_debug, 			proc_read_debug},
	{"param",			NULL, 	NULL, 	proc_write_prm, 			proc_read_prm},
	{"param_2pass",	    NULL, 	NULL, 	proc_write_prm2, 			proc_read_prm2},
	{"enhance_nr",      NULL,   NULL,   proc_write_enhanceNR,       proc_read_enhanceNR},
};

/*
 * debug
 */
static int proc_read_debug(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int     len = 0, dev, queued_size;

    len += sprintf(page + len, "\n");

    if (priv.eng_num == 0) {
        len += sprintf(page + len, "No FTDI220 Engine ready!\n");
        goto exit;
    }

    len += sprintf(page + len, "Total Jobs: %d\n", priv.jobseq);
    for (dev = 0; dev < priv.eng_num; dev ++) {
        len += sprintf(page + len,"Engine%d: busy bit = %d\n", priv.engine[dev].dev, priv.engine[dev].busy);
        len += sprintf(page + len,"    Register:     0x%x\n",priv.engine[dev].ftdi220_reg);
        len += sprintf(page + len,"    IRQ:          %d\n",priv.engine[dev].irq);
        len += sprintf(page + len,"    Job Finished: %d\n",priv.engine[dev].handled);
        len += sprintf(page + len,"    Enhance3dNR:  %d\n",priv.engine[dev].enhance_nr_cnt);
        queued_size = atomic_read(&priv.engine[dev].list_cnt);
        len += sprintf(page + len,"    Queued jobs:  %d \n", queued_size);
        len += sprintf(page + len,"---------------------------------- \n\n");
    }

	len += sprintf(page + len, "eng_node_count %d\n", priv.eng_node_count);

#ifdef VIDEOGRAPH_INC
    printeng_vg(page, &len, data);
#endif

exit:
    *eof = 1;       //end of file
    return len;
}

#ifdef VIDEOGRAPH_INC
extern unsigned int  debug_value;
#endif

int proc_write_debug(struct file *file, const char *buffer, unsigned long count, void *data)
{
    unsigned char value[60];
    u32 tmp;

    if (copy_from_user(value, buffer, count))
        return 0;

    sscanf(value, "%x\n", &tmp);
    printk("value = 0x%x \n", tmp);

#ifdef VIDEOGRAPH_INC
    debug_value = tmp;
#endif

    return count;
}

/*
 * 3DNR enahance
 */
static int proc_read_enhanceNR(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int     len = 0;

    len += sprintf(page + len, "\n");
    len += sprintf(page + len, "1 for enabled, 0 for disabled. \n");
    len += sprintf(page + len, "Ehance debug_enhance_3dnr: %d\n", dbg_enhance_3dnr);
    *eof = 1;       //end of file
    return len;
}

int proc_write_enhanceNR(struct file *file, const char *buffer, unsigned long count, void *data)
{
    unsigned char value[20];

    if (copy_from_user(value, buffer, count))
        return 0;

    sscanf(value, "%x\n", &dbg_enhance_3dnr);
    printk("debug_enhance_3dnr = 0x%x \n", dbg_enhance_3dnr);

    return count;
}

/* INIT function
 */

void proc_init(void)
{
	unsigned int i = 0;
	unsigned int proc_count = sizeof(g_prm_proc)/sizeof(prm_proc_t);
	umode_t mode = S_IRUGO | S_IXUGO;

	for (i = 0; i < proc_count; i++)
	{

		mode = S_IRUGO | S_IXUGO;
		if (i == 0)
		{
			mode |= S_IFDIR;
		}
		if (i > 0)
		{
			g_prm_proc[i].parent = g_prm_proc[0].proc_entry;
		}
		g_prm_proc[i].proc_entry = create_proc_entry(g_prm_proc[i].proc_name, mode, g_prm_proc[i].parent);
		g_prm_proc[i].proc_entry->read_proc = g_prm_proc[i].p_proc_read;
		g_prm_proc[i].proc_entry->write_proc = g_prm_proc[i].p_proc_write;
	}
}


/* proc remove function
 */

void proc_remove(void)
{
	unsigned int i = 0;
	unsigned int proc_count = sizeof(g_prm_proc)/sizeof(prm_proc_t);
	for (i = proc_count; i > 0; i--)
	{
		remove_proc_entry(g_prm_proc[i - 1].proc_name, g_prm_proc[i - 1].parent);
	}
}


