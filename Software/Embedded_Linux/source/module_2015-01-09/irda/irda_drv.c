/*
 * Revision History
 * VER1.1 2014/09/17 ADD HL_PARAM for insmod to adjust ho lo param
 */
#include <linux/proc_fs.h>
#include "platform.h"
#include "irda_drv.h"
#include "irda_dev.h"
#include "irda_api.h"
#include "irda_specific.h"
#include <asm-generic/gpio.h>


#define IRDA_VER "VER:1.1"

static unsigned int protocol = IRDET_NEC;   // 0:NEC, 1:RCA, 2:Sharp, 3:Philips RC-5, 4:Nokia NRC17
module_param(protocol, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(protocol, "0:NEC"/*", 1:RCA, 2:Sharp, 3:Philips RC-5, 4:Nokia NRC17"*/);   //currently, only support NEC protocol
#ifdef CONFIG_PLATFORM_GM8287
static unsigned int gpio_num = 16; 
#elif defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
static unsigned int gpio_num = 3; 
#else
static unsigned int gpio_num = 9;   
#endif
module_param(gpio_num, uint, S_IRUGO|S_IWUSR);

#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
MODULE_PARM_DESC(gpio_num, "default: 3");
#elif defined(CONFIG_PLATFORM_GM8287)
MODULE_PARM_DESC(gpio_num, "default: 16");
#else
MODULE_PARM_DESC(gpio_num, "default: 9");
#endif

ushort hl_param[3] = {[0 ... (3 - 1)] = 0};
module_param_array(hl_param, ushort, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(hl_param, "irda hi lo_h lo_l parameter");

static struct proc_dir_entry *irda_proc_root = NULL;
static struct proc_dir_entry *irda_hl_info = NULL;


/* function for register device */
void device_release(struct device *dev);

/* function for register driver */
int driver_probe(struct platform_device * pdev);
int __devexit driver_remove(struct platform_device *pdev);

/* function for register file operation */
static int file_open(struct inode *inode, struct file *filp);
static int file_release(struct inode *inode, struct file *filp);
static long file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static ssize_t file_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static unsigned int file_poll(struct file *filp, poll_table *wait);

static struct resource _dev_resource[] = {
	[0] = {
		.start = DEV_PA_START,
		.end = DEV_PA_END,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DEV_IRQ_START,
		.end = DEV_IRQ_END,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device _device = {
	.name = DEV_NAME,
	.id = -1, /* "-1" to indicate there's only one. */
	#ifdef HARDWARE_IS_ON
	.num_resources = ARRAY_SIZE(_dev_resource),
	.resource = _dev_resource,
	#endif
	.dev  = {
		.release = device_release,
	},
};

/*
 * platform_driver
 */
static struct platform_driver _driver = {
    .probe = driver_probe,
    .remove = driver_remove,
    .driver = {
           .owner = THIS_MODULE,
           .name = DEV_NAME,
           .bus = &platform_bus_type,
    },
};

static struct file_operations _fops = {
	.owner 			= THIS_MODULE,
    .unlocked_ioctl = file_ioctl,
	.open 			= file_open,
	.release 		= file_release,
	.read			= file_read,
	.poll       	= file_poll,
};

static int proc_dump_hl_param_info(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;
    struct dev_data* p_dev_data = NULL;
    
    p_dev_data = (struct dev_data *)platform_get_drvdata(&_device); 

    len += sprintf(page + len, "irda param0: 0x%08x \n", Irdet_GetParam0((u32)p_dev_data->io_vadr));
    len += sprintf(page + len, "irda param1: 0x%08x \n", Irdet_GetParam1((u32)p_dev_data->io_vadr));
    return len;
}


void drv_proc_init(void)
{
    struct proc_dir_entry *p;

    p = create_proc_entry("irda", S_IFDIR | S_IRUGO | S_IXUGO, NULL);

	if (p == NULL) {
        panic("Fail to create proc irda root!\n");
    }

    irda_proc_root = p;

    /*
     * debug message
     */
    irda_hl_info = create_proc_entry("hl_param", S_IRUGO, irda_proc_root);

	if (irda_hl_info == NULL)
        panic("Fail to create irda_hl_info!\n");
    irda_hl_info->read_proc = (read_proc_t *) proc_dump_hl_param_info;
    irda_hl_info->write_proc = NULL;
}

void drv_proc_release(void)
{
    if (irda_hl_info != NULL)
        remove_proc_entry(irda_hl_info->name, irda_proc_root);

    if (irda_proc_root != NULL)
		remove_proc_entry(irda_proc_root->name, NULL);
}

void device_release(struct device *dev)
{
    PRINT_FUNC();
    return;
}


int register_device(struct platform_device *p_device)
{
    int ret = 0;

    if (unlikely((ret = platform_device_register(p_device)) < 0)) 
    {
        printk("%s fails: platform_device_register not OK\n", __FUNCTION__);
    }
    
    return ret;    
}

void unregister_device(struct platform_device *p_device)
{
    
    platform_device_unregister(p_device);
}

int register_driver(struct platform_driver *p_driver)
{
    PRINT_FUNC();
	return platform_driver_register(p_driver);
}



void unregister_driver(struct platform_driver *p_driver)
{
    platform_driver_unregister(p_driver);
}


/*
 * register_cdev
 */ 
int register_cdev(struct dev_data* p_dev_data)
{
    int ret = 0;

    /* alloc chrdev */
    ret = alloc_chrdev_region(&p_dev_data->dev_num, 0, 1, DEV_NAME);
    if (unlikely(ret < 0)) {
        printk(KERN_ERR "%s:alloc_chrdev_region failed\n", __func__);
        goto err1;
    }
    
    cdev_init(&p_dev_data->cdev, &_fops);
    p_dev_data->cdev.owner = THIS_MODULE;

    ret = cdev_add(&p_dev_data->cdev, p_dev_data->dev_num, 1);
    if (unlikely(ret < 0)) {
        PRINT_E(KERN_ERR "%s:cdev_add failed\n", __func__);
        goto err2;
    }

    /* create class */
    p_dev_data->class = class_create(THIS_MODULE, CLS_NAME);
	if (IS_ERR(p_dev_data->class))
	{
        PRINT_E(KERN_ERR "%s:class_create failed\n", __func__);
        goto err3;
	}

    
    /* create a node in /dev */
	p_dev_data->p_device = device_create(
        p_dev_data->class,              /* struct class *class */
        NULL,                           /* struct device *parent */
        p_dev_data->cdev.dev,    /* dev_t devt */
	    p_dev_data,                     /* void *drvdata, the same as platform_set_drvdata */
	    DEV_NAME                 /* const char *fmt */
	);
    
    PRINT_FUNC();
    return 0;
    
    err3:
        cdev_del(&p_dev_data->cdev);
    
    err2:
        unregister_chrdev_region(p_dev_data->dev_num, 1);
        
    err1:
        return ret;
}


/*
 * unregister_cdev
 */ 
void unregister_cdev(struct dev_data* p_dev_data)
{
    PRINT_FUNC();

    device_destroy(p_dev_data->class, p_dev_data->cdev.dev);
    
    class_destroy(p_dev_data->class);
    
    cdev_del(&p_dev_data->cdev);
    
    unregister_chrdev_region(p_dev_data->dev_num, 1);
    
}

static void* dev_data_alloc_specific(struct dev_data* p_dev_data)
{
	struct dev_specific_data_t* p_data = &p_dev_data->dev_specific_data;

    spin_lock_init(&p_data->lock);

	/* setting fifo */
    p_data->queue_len = 16;
    if (unlikely(kfifo_alloc(
		&p_data->fifo, 
		p_data->queue_len * sizeof(irda_pub_data), 
		GFP_KERNEL))
	)
	{
		panic("kfifo_alloc fail in %s\n", __func__);
        goto err0;
	}
    //kfifo_reset(&p_data->fifo);
    init_waitqueue_head(&p_data->wait_queue);
	
	/* setting work and workqueue */
    INIT_DELAYED_WORK(&p_data->work_st, restart_hardware);
    p_data->delay_jiffies = DEFAULT_DELAY_DURATION;
	
    init_MUTEX(&p_data->oper_sem);

	p_data->protocol = protocol;
	p_data->gpio_num = gpio_num; /* use gpio 9 */
	printk(" * dev_data_alloc_specific done\n");
    return p_data;


    err0:
    return NULL;
}

static void* dev_data_alloc(void)
{
    struct dev_data* p_dev_data = NULL;
        
    /* alloc drvdata */
    p_dev_data = kzalloc(sizeof(struct dev_data), GFP_KERNEL);

    if (unlikely(p_dev_data == NULL))
    {
        PRINT_E("%s Failed to allocate p_dev_data\n", __FUNCTION__);        
        goto err0;
    }

	if (unlikely(dev_data_alloc_specific(p_dev_data) == NULL))
    {
        PRINT_E("%s Failed to allocate p_dev_data\n", __FUNCTION__);        
        goto err1;
    }

   	DRV_COUNT_RESET();
	return p_dev_data;
	
	err1:
	    kfree (p_dev_data);
		p_dev_data = NULL;
    err0:
        return p_dev_data;
}

static int dev_data_free_specific(struct dev_data* p_dev_data)
{
	struct dev_specific_data_t* p_data = &p_dev_data->dev_specific_data;
    kfifo_free(&p_data->fifo);
	return 0;
}

static int dev_data_free(struct dev_data* p_dev_data)
{      
	dev_data_free_specific(p_dev_data);
    /* free drvdata */
    kfree(p_dev_data);
    p_dev_data = NULL;
    
	return 0;
}

static irqreturn_t drv_interrupt(int irq, void *base)
{
    unsigned int    	status;
	unsigned int 		len;
	//unsigned long 		cpu_flags;
    irda_pub_data       new_data;
    irda_pub_data       debug_data;
    struct dev_data*	p_dev_data = (struct dev_data*)base;
    struct dev_specific_data_t*	p_data = &p_dev_data->dev_specific_data;
	
    new_data.val = 0;
    new_data.repeat = 0;
    debug_data.val = 0;
    debug_data.repeat = 0;
	
    status = Irdet_GetStatus((u32)p_dev_data->io_vadr);
    Irdet_SetStatus((u32)p_dev_data->io_vadr, IRDET_STS_IR_INT|IRDET_STS_IR_REPEAT);
    if(status & IRDET_STS_IR_INT)
    {
        new_data.val = Irdet_GetData((u32)p_dev_data->io_vadr);
    }
    if(status & IRDET_STS_IR_REPEAT)
        new_data.repeat = 1;
    else
        new_data.repeat = 0;



	len = kfifo_in(&p_data->fifo, &new_data, sizeof(irda_pub_data));
	if (unlikely(!len))
	    printk("Put irdet data into filo failed. %d\n", len);



    wake_up(&p_data->wait_queue);

    //delay to restart hardware
    schedule_delayed_work(&p_data->work_st,p_data->delay_jiffies);

    return IRQ_HANDLED;
}

irqreturn_t drv_isr(int irq, void *dev)
{

	return 	drv_interrupt(irq, dev);
}

int init_hardware(struct dev_data *p_dev_data)
{
    struct dev_specific_data_t *p_data = &p_dev_data->dev_specific_data;
	int ret = 0;
	
	/****************************************************/
	/**** start to implement your init_hardware here ****/
	/****************************************************/
	
    ret = scu_probe(&p_dev_data->scu);
    if (unlikely(ret != 0)) 
    {
        PRINT_E("%s fails: scu_probe not OK %d\n", __FUNCTION__, ret);
        goto err0;
    }
    irda_SetPinMux(p_data->gpio_num);

	/* set selected gpio as input pin */
    if ((ret = gpio_request(p_data->gpio_num, DEV_NAME)) != 0) {
        printk(KERN_ERR "%s: gpio_request %u failed %d\n", __func__, p_data->gpio_num, ret);
        goto err0;
    }
    if ((ret = gpio_direction_input(p_data->gpio_num)) != 0) {
        printk(KERN_ERR "%s: gpio_direction_input failed %d\n", __func__, ret);
        goto err0;
		
    }	

	/* set up hardware */
    Irdet_Set_HL_Param(hl_param[0],hl_param[1],hl_param[2]);
	printk(" * gpio %d, protocol %d ver %s\n", p_data->gpio_num,p_data->protocol,IRDA_VER);
	Irdet_HwSetup(
		(u32)p_dev_data->io_vadr, 
		p_data->protocol, 
		p_data->gpio_num
	);

	return 0;
	
	err0:
		return ret;
}

void exit_hardware(struct dev_data *p_dev_data)
{
    struct dev_specific_data_t *p_data = &p_dev_data->dev_specific_data;

    PRINT_FUNC();
	
	/****************************************************/
	/**** start to implement your exit_hardware here ****/
	/****************************************************/
	
	/* free gpio */
	gpio_free(p_data->gpio_num);
	
    /* remove pmu */
    scu_remove(&p_dev_data->scu);
}

int driver_probe(struct platform_device * pdev)
{
    struct dev_data *p_dev_data = NULL;
    #ifdef HARDWARE_IS_ON
    struct resource *res = NULL;  
    #endif
    struct dev_specific_data_t *p_data = NULL;
  
    int ret = 0;
    PRINT_FUNC();
    
    /* 1. alloc and init device driver_Data */
    p_dev_data = dev_data_alloc();
    if (unlikely(p_dev_data == NULL)) 
    {
        printk("%s fails: kzalloc not OK", __FUNCTION__);
        goto err1;
    }
    p_data = &p_dev_data->dev_specific_data;
	
    /* 2. set device_drirver_data */
    platform_set_drvdata(pdev, p_dev_data);

    p_dev_data->id = pdev->id;    
   
    /* 3. request resource of base address */
    #ifdef HARDWARE_IS_ON
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

    if (unlikely((!res))) 
    {
        PRINT_E("%s fails: platform_get_resource not OK", __FUNCTION__);
        goto err2;
    }

    p_dev_data->io_size = res->end - res->start + 1;
    p_dev_data->io_padr = (void*)res->start;

    if (unlikely(!request_mem_region(res->start, p_dev_data->io_size, pdev->name))) {
        PRINT_E("%s fails: request_mem_region not OK", __FUNCTION__);
        goto err2;
    }

    p_dev_data->io_vadr = (void*) ioremap((uint32_t)p_dev_data->io_padr, p_dev_data->io_size);

    
    if (unlikely(p_dev_data->io_vadr == 0)) 
    {
        PRINT_E("%s fails: ioremap_nocache not OK", __FUNCTION__);
        goto err3;
    }

    /* 4. request_irq */
    p_dev_data->irq_no = platform_get_irq(pdev, 0);
    if (unlikely(p_dev_data->irq_no < 0)) 
    {
        PRINT_E("%s fails: platform_get_irq not OK", __FUNCTION__);
        goto err3;
    }

    ret = request_irq(
        p_dev_data->irq_no, 
        drv_isr, 
        0,
        DEV_NAME, 
        p_dev_data
    ); 

    if (unlikely(ret != 0)) 
    {
        PRINT_E("%s fails: request_irq not OK %d\n", __FUNCTION__, ret);
        goto err4;
    }
    
    
    /* 6. init pmu */
	ret = init_hardware(p_dev_data);
    if (unlikely(ret) < 0) 
    {
        PRINT_E("%s fails: init_hardware not OK\n", __FUNCTION__);
        goto err4;
    }
	#endif
    

    
    /* 7. register cdev */
    ret = register_cdev(p_dev_data);
    if (unlikely(ret) < 0) 
    {
        PRINT_E("%s fails: register_cdev not OK\n", __FUNCTION__);
        goto err5;
    }


    /* 8. print probe info */
    #if 1
    printk(" * %s done, irq_no %d io_vadr 0x%08X, io_padr 0x%08X 0x%08X\n", 
    __FUNCTION__, 
    p_dev_data->irq_no,
    (unsigned int)p_dev_data->io_vadr, 
    (unsigned int)p_dev_data->io_padr, 
    (unsigned int)p_dev_data
    );
    #endif

    return ret;
    

    #ifdef HARDWARE_IS_ON
    err5:    
        exit_hardware(p_dev_data);
		
    err4:
        free_irq(p_dev_data->irq_no, p_dev_data);
        
    err3:
        release_mem_region((unsigned int)p_dev_data->io_padr, p_dev_data->io_size);

    err2:
        platform_set_drvdata(pdev, NULL);
    #endif
        

    err1:
        return ret;
}

int __devexit driver_remove(struct platform_device *pdev)
{      
	struct dev_data* p_dev_data = NULL;

    PRINT_FUNC();
	
	p_dev_data = (struct dev_data *)platform_get_drvdata(pdev);	

    PRINT_I(" * %s p_dev_data 0x%08X\n", __FUNCTION__, (unsigned int)p_dev_data);

	
    /* unregister cdev */
    unregister_cdev(p_dev_data);
    
    #ifdef HARDWARE_IS_ON
 	/* exit hardware */	
	exit_hardware(p_dev_data);

    /* release resource */
    iounmap((void __iomem *)p_dev_data->io_vadr); 
    release_mem_region((u32)p_dev_data->io_padr, p_dev_data->io_size);

    /* free irq */
    free_irq(p_dev_data->irq_no, p_dev_data);
    #endif
    
	/* free device memory, and set drv data as NULL	*/
	platform_set_drvdata(pdev, NULL);

    /* free device driver_data */
	dev_data_free(p_dev_data);
	return 0;
}



static int __init module_init_func(void)
{
    int ret = 0;  

    printk(" *************************************\n");
    printk(" * Welcome to use %s,        *\n", DEV_NAME );
    printk(" *************************************\n");

    PRINT_FUNC();   
    
    
    /* register platform device */
    ret = register_device(&_device);
    if (unlikely(ret) < 0) 
    {
        PRINT_E("%s fails: register_device not OK\n", __FUNCTION__);
        goto err1;
    }

    /* register platform driver     
     * probe will be done immediately after platform driver is registered 
     */
    ret = register_driver(&_driver);
    if (unlikely(ret) < 0) 
    {
        PRINT_E("%s fails: register_driver not OK\n", __FUNCTION__);
        goto err2;
    }
    
    drv_proc_init();
    
    return 0;

        
    err2:
        unregister_device(&_device);		

    err1:
    return ret;

}

static void __exit module_exit_func(void)
{

    printk(" ************************************************\n");
    printk(" * Thank you to use %s, 20130325, goodbye *\n", DEV_NAME );
    printk(" ************************************************\n");

    PRINT_FUNC();   
    
    
    /* unregister platform driver */     
    unregister_driver(&_driver);


	/* register platform device */
    unregister_device(&_device);

    drv_proc_release();
}

static int file_open_specific(struct dev_data* p_dev_data)
{
	struct dev_specific_data_t* p_data = &p_dev_data->dev_specific_data;
    return Irdet_HwSetup((u32)p_dev_data->io_vadr, p_data->protocol, p_data->gpio_num);
}

static int file_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	
	struct flp_data* p_flp_data = NULL;	
	struct dev_data* p_dev_data = NULL;
	
    /* set filp */
    p_dev_data = container_of(inode->i_cdev, struct dev_data, cdev);
    
	p_flp_data = kzalloc(sizeof(struct flp_data), GFP_KERNEL);
	p_flp_data->p_dev_data = p_dev_data;

	
    /* increase driver count */	
	DRV_COUNT_INC(); 

	/* driver specific open */
	file_open_specific(p_dev_data);
	
	/* assign to private_data */
    filp->private_data = p_flp_data;	


    PRINT_I("\n");
	if (unlikely(((unsigned int)p_dev_data != (unsigned int)inode->i_cdev)))
	{
    	PRINT_I("%s p_dev_data 0x%08X 0x%08X\n", __FUNCTION__, (unsigned int)p_dev_data, (unsigned int)inode->i_cdev);
    	PRINT_I("%s p_flp_data 0x%08X\n", __FUNCTION__, (unsigned int)p_flp_data);
		ret = -1;
	}
    return ret;
}


static long file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    
    long ret = 0;

	struct flp_data* p_flp_data = filp->private_data;	
	struct dev_data* p_dev_data = p_flp_data->p_dev_data;
	struct dev_specific_data_t* p_data = &p_dev_data->dev_specific_data;


    down(&p_data->oper_sem);

    switch(cmd)
    {
      case IRDET_SET_DELAY_INTERVAL:
        {
        int   ms;
	    if (unlikely(copy_from_user((void *)&ms, (void *)arg, sizeof(int))))
	    {
			ret = -EFAULT;
			break;
	    }

        if(ms>0 && ms <=1000)
            p_data->delay_jiffies = ms;
        else
            ret = -EFAULT;
        break;
      }
      default:
        printk("%s cmd(0x%x) no define!\n", __func__, cmd);
        break;
    }

    up(&p_data->oper_sem);

    return ret;
}



static ssize_t file_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
 	unsigned long 		cpu_flags;
    int         ret;
	int len;
	irda_pub_data  	data;
	struct flp_data* p_flp_data = filp->private_data;	
	struct dev_data* p_dev_data = p_flp_data->p_dev_data;
	struct dev_specific_data_t* p_data = &p_dev_data->dev_specific_data;

    if(down_interruptible(&p_data->oper_sem))
        return -ERESTARTSYS;

	spin_lock_irqsave(&p_data->lock, cpu_flags);
    if(kfifo_len(&p_data->fifo)==0)
    {
    	ret = 0;
		goto exit;
    }
    
 	if (unlikely((len = 
		kfifo_out(&p_data->fifo, &data, sizeof(irda_pub_data))<= 0)))
	{
    	ret = -ERESTARTSYS;
		goto exit;
	}

    if(copy_to_user(buf, &data, sizeof(irda_pub_data)))
    {
    	ret = -ERESTARTSYS;
		goto exit;
    }

    ret=sizeof(irda_pub_data);
	exit:
 	spin_unlock_irqrestore(&p_data->lock, cpu_flags);
    up(&p_data->oper_sem);

    return ret;
}

static unsigned int file_poll(struct file *filp, poll_table *wait)
{
    unsigned int mask = 0;
	struct flp_data* p_flp_data = filp->private_data;	
	struct dev_data* p_dev_data = p_flp_data->p_dev_data;
	struct dev_specific_data_t* p_data = &p_dev_data->dev_specific_data;
 	unsigned long 		cpu_flags;

	//printk("%s\n", __func__);
    if(down_interruptible(&p_data->oper_sem))
        return -ERESTARTSYS;
	
    poll_wait(filp, &p_data->wait_queue, wait);
	spin_lock_irqsave(&p_data->lock, cpu_flags);
    if(kfifo_len(&p_data->fifo))
        mask |= POLLIN | POLLRDNORM;
 	spin_unlock_irqrestore(&p_data->lock, cpu_flags);
    up(&p_data->oper_sem);

	return mask;

}
static int file_release(struct inode *inode, struct file *filp)
{
	struct flp_data* p_flp_data = filp->private_data;	

    /* remove this filp from the asynchronously notified filp's */

    kfree(p_flp_data);

    filp->private_data = NULL;
    
    return 0;
}


module_init(module_init_func);
module_exit(module_exit_func);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("GM fasync test");
MODULE_LICENSE("GPL");


