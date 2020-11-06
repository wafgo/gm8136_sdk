///#define _DEBUG_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/hdreg.h>    /* HDIO_GETGEO */
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/syscalls.h>
#include <linux/miscdevice.h>
#include <linux/vmalloc.h>
#include <mach/fmem.h>
#include <asm/cacheflush.h>
#include <mach/platform/board.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/kthread.h>

#if (LINUX_VERSION_CODE == KERNEL_VERSION(3,3,0))
#include <mach/fmem.h>
#endif

#if (LINUX_VERSION_CODE == KERNEL_VERSION(2,6,14))
#include <frammap/frammap.h>
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28))
#include <frammap/frammap_if.h>
#endif

#include <ivs_ioctl/ivs_ioctl.h>
#include "fdt_ioctl.h"
#include "fdt_alg.h"

#ifdef _DEBUG_
#if (HZ == 1000)
    #define get_gm_jiffies()                (jiffies)
#else
    #include <mach/gm_jiffies.h>
#endif
#endif

#define FDT_MINOR           31

#define ERR_MEM_ALLOC       (-1)
#define ERR_INVALID_PARAM   (-2)
#define ERR_COPY_FROM_USER  (-3)
#define ERR_COPY_TO_USER    (-4)
#define ERR_CREATE_THREAD   (-5)
#define ERR_IVS             (-6)

#define MAX_DETECTIONS      (65535)  ///< Maximum number of output coordinates from IVS-CC hardware.
#define MAX_SCALINGS        (20)     ///< Maximum scaling times.
#define MIN_SCL_IMG_W       (64)     ///< Minimum scaled image width.
#define MIN_SCL_IMG_H       (36)     ///< Minimum scaled image height.
#define MAX_SEN             (10)     ///< Maximum sensitivity.
#define DT_WIN_W            (20)     ///< Detection window width and height.
#define INPUT_BUF_SIZE      (cap_width * cap_height << 1) ///< Buffer size of the scaler for one frame.


#define ALIGN4_UP(x)        (((x + 3) >> 2) << 2)
#define ALIGN2_UP(x)        (((x + 1) >> 1) << 1)


/// Structure of face detection runtime variables.
typedef struct _fdt_rt_t {
    TRect *face_buf;
    TObjGrp *face_grp_buf;
    TAntiVarRT av_rt;
} fdt_rt_t;

/// Structure of cas_scaling parameters.
typedef struct vpdCasScaling {
    int             cap_vch;            ///< User channel number
    unsigned int    ratio;              ///< Value range:80~91, it means 80%~91%
    unsigned int    first_width;        ///< The width of first frame, input by AP
    unsigned int    first_height;       ///< The height of first frame, input by AP
    char            *yuv_buf;           ///< Allocated buffer by AP
    unsigned int    yuv_buf_len;        ///< The length of allocated buffer
    int             timeout_millisec;   ///< Timeout value.
} vpdCasScaling_t;


/* Platform spec global parameters */
static int cap_width = 1920;                      ///< Maximum capture width.
module_param(cap_width, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(cap_width, "maximum capture width");
static int cap_height = 1088;                     ///< Maximum capture height
module_param(cap_height, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(cap_height, "maximum capture height");

unsigned char *gVirt = 0;     ///< Preallocated memory.
unsigned int gPhy = 0;        ///< Preallocated memory.
struct semaphore gMutex;      ///< Mutex for the detection command.
struct semaphore gReadMutex;  ///< Mutex for reading the input buffer.
struct semaphore gWriteMutex; ///< Mutex for writing the input buffer.
unsigned int gBufSize;        ///< Preallocated memory size.
vpdCasScaling_t gScalerParam; ///< Scaler parameters.
struct task_struct *gpInputThread = NULL; ///< Input thread pointer.
int gInputBufIdx = 0;


extern int api_get_cas_scaling(vpdCasScaling_t *cas_scaling);


int input_thread(void *data)
{
    struct vpdCasScaling *pParam = (struct vpdCasScaling *)data;
    int ret;
#ifdef _DEBUG_
    unsigned long stamp;
#endif

    set_current_state(TASK_INTERRUPTIBLE);
    //set_user_nice(current, wq->task_nice);
    while (!kthread_should_stop()) {
        down(&gWriteMutex); // get writing semaphore
#ifdef _DEBUG_
        stamp = get_gm_jiffies();
#endif
        ret = api_get_cas_scaling(pParam); // get the image pyramid
#ifdef _DEBUG_
        printk("scaling time = %d\n", get_gm_jiffies() - stamp);
#endif
        up(&gReadMutex); // release reading semaphore

        if (ret < 0)
            printk("Errors in api_get_cas_scaling().\n");

        schedule();
        set_current_state(TASK_INTERRUPTIBLE);
        //msleep(10);
    }
    __set_current_state(TASK_RUNNING);

    return 0;
}

/** Checks and corrects face detection parameters.
 * \param[in] pParam  Pointer to the parameters.
 */
int check_fdt_param(fdt_param_t *pParam)
{
    int ret = 0;
    unsigned int ratio;
    unsigned int first_width;
    unsigned int first_height;
    int scaling_timeout_ms;
    unsigned char sensitivity;
    char delayed_vanish_frm_num;
    char min_frm_num;
    char max_frm_num;

    pParam->face_num = 0;

    // ratio: 80 ~ 91
    ratio = max((unsigned int)80, min(pParam->ratio, (unsigned int)91));
    if (ratio != pParam->ratio) {
        printk("Invalid parameter: ratio = %d\n", pParam->ratio);
        ret = ERR_INVALID_PARAM;
    }
    // first_width
    first_width = max((unsigned int)MIN_SCL_IMG_W, min(ALIGN4_UP(pParam->first_width), (unsigned int)320));
    if (first_width != pParam->first_width) {
        printk("Invalid parameter: first_width = %d\n", pParam->first_width);
        ret = ERR_INVALID_PARAM;
    }
    // first_height
    first_height = max((unsigned int)MIN_SCL_IMG_H, min(ALIGN2_UP(pParam->first_height), (unsigned int)320));
    if (first_height != pParam->first_height) {
        printk("Invalid parameter: first_height = %d\n", pParam->first_height);
        ret = ERR_INVALID_PARAM;
    }
    // scaling_timeout_ms
    scaling_timeout_ms = max((int)0, pParam->scaling_timeout_ms);
    if (scaling_timeout_ms != pParam->scaling_timeout_ms) {
        printk("Invalid parameter: scaling_timeout_ms = %d\n", pParam->scaling_timeout_ms);
        ret = ERR_INVALID_PARAM;
    }
    // sensitivity
    sensitivity = (unsigned char)max((int)0, min((int)pParam->sensitivity, (int)9));
    if (sensitivity != pParam->sensitivity) {
        printk("Invalid parameter: sensitivity = %d\n", pParam->sensitivity);
        ret = ERR_INVALID_PARAM;
    }

    // fdt_av_param_t.delayed_vanish_frm_num
    delayed_vanish_frm_num = (char)max((int)0, min((int)pParam->av_param.delayed_vanish_frm_num, (int)127));
    if (delayed_vanish_frm_num != pParam->av_param.delayed_vanish_frm_num) {
        printk("Invalid parameter: av_param.delayed_vanish_frm_num = %d\n", pParam->av_param.delayed_vanish_frm_num);
        ret = ERR_INVALID_PARAM;
    }
    // fdt_av_param_t.min_frm_num
    min_frm_num = (char)max((int)1, min((int)pParam->av_param.min_frm_num, (int)16));
    if (min_frm_num != pParam->av_param.min_frm_num) {
        printk("Invalid parameter: av_param.min_frm_num = %d\n", pParam->av_param.min_frm_num);
        ret = ERR_INVALID_PARAM;
    }
    // fdt_av_param_t.max_frm_num
    max_frm_num = (char)max((int)1, min((int)pParam->av_param.max_frm_num, (int)16));
    if (max_frm_num != pParam->av_param.max_frm_num) {
        printk("Invalid parameter: av_param.max_frm_num = %d\n", pParam->av_param.max_frm_num);
        ret = ERR_INVALID_PARAM;
    }
    return ret;
}

/** Creates runtime variables of face detection.
 * \param[in] pParam  Pointer to the parameters.
 * \return  Returns 0 if success. Returns negative number if error occur.
 */
int fdt_create_runtime(fdt_param_t *pParam)
{
    fdt_rt_t *pRT = (fdt_rt_t*)vmalloc(sizeof(fdt_rt_t));

    if (!pRT)
        return ERR_MEM_ALLOC;
    pRT->face_buf = NULL;
    pRT->face_grp_buf = NULL;

    pRT->face_buf = (TRect*)vmalloc(MAX_DETECTIONS * sizeof(TRect));
    if (!pRT->face_buf)
        goto err_exit;
    pRT->face_grp_buf = (TObjGrp*)vmalloc(pParam->max_face_num * sizeof(TObjGrp));
    if (!pRT->face_grp_buf)
        goto err_exit;
    if (pParam->av_param.en)
        if (OD_CreateAntiVariationRT(&pRT->av_rt, (const int)pParam->max_face_num) < 0)
            goto err_exit;

    pParam->rt = (void*)pRT;

    return 0;

err_exit:

    if (pRT->face_buf)
        vfree(pRT->face_buf);
    if (pRT->face_grp_buf)
        vfree(pRT->face_grp_buf);
    if (pRT)
        vfree(pRT);

    return ERR_MEM_ALLOC;
}

/** Releases runtime variables of face detection.
 * \param[in] pParam  Pointer to the parameters.
 * \return  Returns 0 if success. Returns negative number if error occur.
 */
int fdt_destroy_runtime(fdt_param_t *pParam)
{
    fdt_rt_t *pRT = (fdt_rt_t*)pParam->rt;

    vfree(pRT->face_buf);
    vfree(pRT->face_grp_buf);
    if (pParam->av_param.en)
        OD_DestroyAntiVariationRT(&pRT->av_rt);
    vfree(pRT);

    return 0;
}

/** Grabs an image from cas_scaling, and detects faces within it.
 * \param[in] pParam  Pointer to the parameters.
 * \return  Returns 0 if success. Returns negative number if error occur.
 */
int fdt_detect(fdt_param_t *pParam)
{
    const int inputBufSize = INPUT_BUF_SIZE;
    int imgDim[MAX_SCALINGS][2];
    int scales;
    int offsetCCOut;
    ivs_param_t ivsParam;
    int faceCount = 0;
    int sclIdx;
    int sclW;
    int sclH;
    int sclFaceNum;
    int origW = pParam->source_width;
    int restoreMultiplier;
    int restoreRectW;
    int i;
    int ret = 0;
    unsigned short *pFaces;
    unsigned short *pCCOut;
    fdt_rt_t *pRT = (fdt_rt_t*)pParam->rt;
    TRect *faceBuf = pRT->face_buf;
    TObjGrp *faceGrpBuf = pRT->face_grp_buf;
    TAntiVarParam avParam = {
        //.thMissDetectCount = 3,
        .thMoveRatio = 13,
        .thAbnormalMoveRatio = 128,
        .thAbnormalMoveCount = 2,
        .thWidthChangeRatio = 24,
        .thWidthChangeCount = 2,
        //.minMedianWinSize = 3,
        //.maxMedianWinSize = 9
    };
#ifdef _DEBUG_
    unsigned long sumT;
    unsigned long stamp;
#endif

    // set cascade scaling parameters
    gScalerParam.cap_vch = pParam->cap_vch;
    gScalerParam.ratio = pParam->ratio;
    gScalerParam.first_width = pParam->first_width;
    gScalerParam.first_height = pParam->first_height;
    gScalerParam.yuv_buf_len = inputBufSize;///gBufSize;
    gScalerParam.timeout_millisec = pParam->scaling_timeout_ms;

    // calculate image sizes in the pyramid
    sclW = gScalerParam.first_width;
    sclH = gScalerParam.first_height;
    offsetCCOut = inputBufSize; // CC output to the end of the second input buffer
    scales = 0;
    for (i = 0; (i < MAX_SCALINGS) && (sclW >= MIN_SCL_IMG_W) && (sclH >= MIN_SCL_IMG_H); ++i) {
        imgDim[i][0] = sclW;
        imgDim[i][1] = sclH;
        offsetCCOut += (sclW * sclH << 1);
        sclW = ALIGN4_UP((sclW * gScalerParam.ratio) / 100);
        sclH = ALIGN2_UP((sclH * gScalerParam.ratio) / 100);
        ++scales;
    }

    down(&gMutex); // get semaphore

    // create input thread if it doesn't exist
    if (gpInputThread == NULL) {
        gScalerParam.yuv_buf = (char*)gVirt + (1 - gInputBufIdx) * inputBufSize;
        gpInputThread = kthread_create(input_thread, &gScalerParam, "fdt_input");
        if (IS_ERR(gpInputThread)) {
            printk("FDT: Error in creating kernel thread! \n");
            gpInputThread = NULL;
            return ERR_CREATE_THREAD;
        }
        wake_up_process(gpInputThread);
    }

    // switch buffer address for the scaler
    down(&gReadMutex); // get reading semaphore
    gScalerParam.yuv_buf = (char*)gVirt + gInputBufIdx * inputBufSize;
    gInputBufIdx = 1 - gInputBufIdx;
    up(&gWriteMutex); // release writing semaphore
    wake_up_process(gpInputThread);

    // set ivs parameters
    memset(&ivsParam, 0, sizeof(ivs_param_t));
    ivsParam.input_img_1.faddr = gPhy + gInputBufIdx * inputBufSize;
    ivsParam.img_info.img_format = 2; // packed
    ivsParam.operation_info.integral_cascade = 1;
    ivsParam.output_rlt_1.faddr = gPhy + offsetCCOut;

#ifdef _DEBUG_
    sumT = 0;
#endif

    for (sclIdx = 0; sclIdx < scales; ++sclIdx) {
        sclW = imgDim[sclIdx][0];
        sclH = imgDim[sclIdx][1];
#ifdef _DEBUG_
        printk("fire ii+cc, image dim %dx%d\n", sclW, sclH);
#endif
        // fire ii+cc
        ivsParam.img_info.img_width = sclW;
        ivsParam.img_info.img_height = sclH;
#ifdef _DEBUG_
        stamp = get_gm_jiffies();
#endif
        ivs_cascaded_classifier(&ivsParam);
#ifdef _DEBUG_
        sumT += get_gm_jiffies() - stamp;
#endif
        ivsParam.input_img_1.faddr += (sclW * sclH << 1); // next input image address
        pCCOut = (unsigned short*)(gVirt + offsetCCOut);

        // restore rectangle sizes and save them
        sclFaceNum = ivsParam.output_rlt_1.size >> 2;
        restoreMultiplier = ((int)origW << 6) / sclW;
        restoreRectW = (DT_WIN_W * restoreMultiplier + 32) >> 6;
#ifdef _DEBUG_
        printk("store %d detections\n", sclFaceNum);
#endif
        for (i = faceCount; i < faceCount + sclFaceNum; ++i) {
            faceBuf[i].x = (pCCOut[0] * restoreMultiplier + 32) >> 6;
            faceBuf[i].y = (pCCOut[1] * restoreMultiplier + 32) >> 6;
            faceBuf[i].width = restoreRectW;
            faceBuf[i].height = 1;
            pCCOut += 2;
        }
        faceCount += sclFaceNum;
    }

#ifdef _DEBUG_
    printk("hw fd time = %d\n", sumT);
#endif

    up(&gMutex); // release semaphore

#ifdef _DEBUG_
    printk("face number before grouper = %d\n", faceCount);
    stamp = get_gm_jiffies();
#endif

    // group faces
    faceCount = OD_ObjectGrouper(faceBuf, faceCount, MAX_SEN - pParam->sensitivity, 2, 2, 2, faceGrpBuf, (int)pParam->max_face_num);

#ifdef _DEBUG_
    printk("grouper time = %d\n", get_gm_jiffies() - stamp);
    printk("face number after grouper = %d\n", faceCount);
#endif

    // anti-variation
    if (pParam->av_param.en) {
        avParam.thMissDetectCount = pParam->av_param.delayed_vanish_frm_num;
        avParam.minMedianWinSize = pParam->av_param.min_frm_num;
        avParam.maxMedianWinSize = pParam->av_param.max_frm_num;
        OD_AntiVariation(faceBuf, &faceCount, &avParam, &pRT->av_rt);
#ifdef _DEBUG_
        printk("face number after anti-variation = %d\n", faceCount);
#endif
    }

    // copy faces to user buffer
    pParam->face_num = faceCount;
    pFaces = pParam->faces;
    for (i = 0; i < faceCount; ++i) {
        pFaces[0] = faceBuf[i].x;
        pFaces[1] = faceBuf[i].y;
        pFaces[2] = faceBuf[i].width;
        pFaces[3] = faceBuf[i].height;
        pFaces += 4;
    }

    return ret;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
long fdt_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
int fdt_ioctl( struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
    int ret = 0;
    fdt_param_t fdtParam;

    if ((ret = copy_from_user((void*)&fdtParam, (void*)arg, sizeof(fdt_param_t)))) {
        printk("Copy from user error (%d).\n", ret);
        return ERR_COPY_FROM_USER;
    }

    switch (cmd) {
        case FDT_INIT:
            if ((ret = check_fdt_param(&fdtParam)) < 0)
                return ret;
            if ((ret = fdt_create_runtime(&fdtParam)) < 0) {
                printk("Face detection initialization error (%d).\n", ret);
                return ret;
            }
            break;
        case FDT_DETECT:
            ret = fdt_detect(&fdtParam);
            if (ret < 0) {
                printk("Face detection execution error (%d).\n", ret);
                return ret;
            }

            break;
        case FDT_END:
            if (gpInputThread != NULL) {
                up(&gWriteMutex); // let input thread going to end
                kthread_stop(gpInputThread);
                down(&gReadMutex); // wait for input thread finishing
                gpInputThread = NULL;
            }
            ret = fdt_destroy_runtime(&fdtParam);
            break;
        default:
            printk("Undefined ioctl cmd %x.\n", cmd);
            break;
    }

    if ((ret = copy_to_user((void*)arg, (void*)&fdtParam, sizeof(fdt_param_t)))) {
        printk("Copy to user error (%d).\n", ret);
        return ERR_COPY_TO_USER;
    }

    return ret;
}

int fdt_mmap(struct file *filp, struct vm_area_struct *vma)
{
    int size = vma->vm_end - vma->vm_start;

    vma->vm_pgoff = gPhy >> PAGE_SHIFT;
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    vma->vm_flags |= VM_RESERVED;

    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot)) {
        kfree((void*)gVirt) ;
        return -EAGAIN;
    }
    return 0;
}

int fdt_open(struct inode *inode, struct file *filp)
{
    return 0; /* success */
}

int fdt_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations fdt_fops = {
    .owner = THIS_MODULE,
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
    .unlocked_ioctl = fdt_ioctl,
#else
    .ioctl          = fdt_ioctl,
#endif
    .mmap           = fdt_mmap,
    .open           = fdt_open,
    .release        = fdt_release,
};

static struct miscdevice fdt_dev = {
    .minor          = FDT_MINOR,
    .name           = "fdt",
    .fops           = &fdt_fops,
};

/** This function is called while insert (insmod) the fdt module.
 * \return  Returns 0 if succeed to initialize the module. Otherwise returns a negative value.
 */
static int __init fdt_drv_init(void)
{
    char memName[16];
    struct frammap_buf_info binfo;
    unsigned int ret = 0;

    if (FDT_VER_BRANCH) {
        printk("Face detection version : %d.%d.%d.%d, built @ %s %s\n",
               FDT_VER_MAJOR, FDT_VER_MINOR, FDT_VER_MINOR2, FDT_VER_BRANCH,
               __DATE__, __TIME__);
    }
    else {
        printk("Face detection version : %d.%d.%d, built @ %s %s\n",
               FDT_VER_MAJOR, FDT_VER_MINOR, FDT_VER_MINOR2,
               __DATE__, __TIME__);
    }

    // inititalize driver
    if (misc_register(&fdt_dev) < 0) {
        printk("face_detection misc-register fail");
        return -1;
    }

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
    sema_init(&gMutex, 1);
    sema_init(&gReadMutex, 1);
    sema_init(&gWriteMutex, 1);
#else
    init_MUTEX(&gMutex);
    init_MUTEX(&gReadMutex);
    init_MUTEX(&gWriteMutex);
#endif
    down(&gReadMutex);

    // pre-allocate memory
    gBufSize = INPUT_BUF_SIZE * 2 * 2 + MAX_DETECTIONS * 4;
    strcpy(memName, "face_detection_mem");
    binfo.size = PAGE_ALIGN(gBufSize);
    binfo.align = 4096;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
    binfo.name = memName;
    binfo.alloc_type = ALLOC_NONCACHABLE;
#endif
    ret = frm_get_buf_info(0, &binfo);
    if (ret == 0) {
        gVirt = (unsigned int)binfo.va_addr;
        gPhy = binfo.phy_addr;
    }
    else {
        printk("allocate memory fail");
        misc_deregister(&fdt_dev);
        return -1;
    }

    return 0;
}

/** This function is called while remove (rmmod) the fdt module. */
static void __exit fdt_drv_exit(void)
{
    misc_deregister(&fdt_dev);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
    frm_release_buf_info((void *)gVirt);
#else
{
    struct frammap_buf_info binfo;
    binfo.va_addr = (unsigned int)gVirt;
    binfo.phy_addr = (unsigned int)gPhy;
    frm_release_buf_info(0, &binfo);
}
#endif
    printk("cleanup\n");
}

module_init(fdt_drv_init);
module_exit(fdt_drv_exit);

MODULE_AUTHOR("Grain Media Corp.");
MODULE_LICENSE("Grain Media License");
MODULE_DESCRIPTION("Face detection driver");
