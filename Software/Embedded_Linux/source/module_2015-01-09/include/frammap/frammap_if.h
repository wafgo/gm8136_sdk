/** frammap_if.h
 *
 *  description: the file provides the function calls for memory allocation.
 *
 *  author: Harry Hsu
 *  version: 1.0
 *  date: 2010/9/24
 *
 */
#ifndef __FRAMMAP_IF_H__
#define __FRAMMAP_IF_H__

typedef enum {
    ALLOC_CACHEABLE = 0x05787888,   ///< cachable
    ALLOC_BUFFERABLE,               ///< bufferable, but non-cache
    ALLOC_NONCACHABLE               ///< non-cachable, non-bufferable
} alloc_type_t;

#ifdef __KERNEL__
#define FRM_ID(idx,dev)     (idx)

/*
 * main structure for user interface
 */
typedef struct frammap_buf_info {
	dma_addr_t  phy_addr;   ///< physical address
	void        *va_addr;   ///< virtual address
	u32         size;	    ///< The size you want to allocate
	int         align;      ///< address alignment
	char        *name;      ///< max is NAME_SZ (20bytes)
    alloc_type_t  alloc_type; ///<  If this parameter is not specified, it is noncacheable/nonbufferable
} frammap_buf_t;

typedef enum {
    DDR_ID_SYSTEM   = 0,
    DDR_ID0         = DDR_ID_SYSTEM,
    DDR_ID1         = 1,
    DDR_ID2         = 2,
    DDR_ID3         = 3,
    DDR_ID4         = 4,
    DDR_ID5         = 5,
    DDR_ID_MAX      = 6
} ddr_id_t;

enum rmmp_index {
    FRMIDX_MIN          = 0,
	FRMIDX_H264E_BS     = 1,
	FRMIDX_H264E_MMAP   = 2,
	FRMIDX_H264E_G1     = 3, //SYSINFO+DMACHAIN
	FRMIDX_H264E_G2     = 4, //RECON+REFER
	FRMIDX_MMAP_M1      = 5,
	FRMIDX_MMAP_M2      = 6,
	FRMIDX_H264D_REF    = 7,
	FRMIDX_FFB0_M       = 8,
	FRMIDX_FFB0_S1      = 9,
	FRMIDX_FFB0_S2      = 10,
	FRMIDX_FFB1_M       = 11,
	FRMIDX_FFB1_S1      = 12,
	FRMIDX_FFB1_S2      = 13,
	FRMIDX_SCAL0        = 14,
	FRMIDX_SCAL1        = 15,
	FRMIDX_CAP0         = 16,
	FRMIDX_CAP1         = 17,
	FRMIDX_CAP2         = 18,
	FRMIDX_CAP3         = 19,
	FRMIDX_CAP4         = 20,
	FRMIDX_CAP5         = 21,
	FRMIDX_CAP6         = 22,
	FRMIDX_CAP7         = 23,
	FRMIDX_CAP8         = 24,
	FRMIDX_CAP9         = 25,
	FRMIDX_CAP10        = 26,
	FRMIDX_CAP11        = 27,
	FRMIDX_CAP12        = 28,
	FRMIDX_CAP13        = 29,
	FRMIDX_CAP14        = 30,
	FRMIDX_CAP15        = 31,
	FRMIDX_CAP16        = 32,
// ---------------------------------- //
	FRMIDX_CAP17        = 33,
	FRMIDX_CAP18        = 34,
	FRMIDX_CAP19        = 35,
	FRMIDX_CAP20        = 36,
	FRMIDX_CAP21        = 37,
	FRMIDX_CAP22        = 38,
	FRMIDX_CAP23        = 39,
	FRMIDX_CAP24        = 40,
	FRMIDX_CAP25        = 41,
	FRMIDX_CAP26        = 42,
	FRMIDX_CAP27        = 43,
	FRMIDX_CAP28        = 44,
	FRMIDX_CAP29        = 45,
	FRMIDX_CAP30        = 46,
	FRMIDX_CAP31        = 47,
	FRMIDX_H264E_SYS    = 48,
	FRMIDX_H264E_DMAMASK0 = 49,
	FRMIDX_H264E_DMAMASK1 = 50,
	FRMIDX_FFB2_M       = 51,
	FRMIDX_FFB2_S1      = 52,
	FRMIDX_FFB2_S2      = 53,
	/* Please insert your index here */
	FRMIDX_USR3         = 54,
	FRMIDX_USR4         = 55,
	FRMIDX_USR5         = 56,
	FRMIDX_USR6         = 57,
	FRMIDX_USR7         = 58,
	FRMIDX_USR8         = 59,
	FRMIDX_USR9         = 60,
	FRMIDX_USR10        = 61,
	FRMIDX_USR11        = 62,
	FRMIDX_USR12        = 63,
	FRMIDX_USR13        = 64,
	FRMIDX_USR14        = 65,
	FRMIDX_USR15        = 66,
	FRMIDX_AP0          = 67,
    FRMIDX_AP1          = 68,
	FRMIDX_AP2          = 69,
	FRMIDX_AP3          = 70,
	FRMIDX_MAX          // max value can't exceed 252
};

/*
 * @This function allocates memory from the desingated DDR based on id
 *
 * @function int frm_get_buf_info(u32 id, struct frammap_buf_info* info)
 * @param id indicates the module id in rmmp_index.
 * @param info is needed information. info->size is necessary.
 * @return 0 on success, <0 on error
 */

extern int frm_get_buf_info(u32 id, struct frammap_buf_info* info);

/*
 * @This function deallocate memory. It is corresponding to frm_get_buf_info()
 *
 * @function int frm_release_buf_info(void *vaddr)
 *
 * @param vaddr is is virtual memory
 * @return 0 on success, <0 on error
 */
extern int frm_release_buf_info(void *vaddr);

/*
 * @This function allocates memory from the designated DDR.
 *
 * @function int frm_get_buf_ddr(ddr_id_t  ddr_id, struct frammap_buf_info *info)
 * @param ddr_id indicates the DDR ID directly
 * @param info is needed information. info->size is necessary.
 * @return 0 on success, <0 on error
 * @Note: the info->size will be updated according to the real allocated size
 */
extern int frm_get_buf_ddr(ddr_id_t  ddr_id, struct frammap_buf_info *info);

/*
 * @This function de-allocates memory. It is corresponding to frm_get_buf_ddr().
 *
 * @function int frm_free_buf_ddr(void *vaddr)
 * @param vaddr is is virtual memory
 * @return 0 on success, <0 on error
 */
extern int frm_free_buf_ddr(void *vaddr);

/*
 * @This function is a special purpose function. It is only used for H26E only
 *
 * @function int frm_get_h264e_register(int id, unsigned int *value)
 * @param id: module id
 * @param value: the return value of register content.
 * @return 0 on success, <0 on error
 */
extern int frm_get_h264e_register(int id, unsigned int *value);

/**
 * @brief to resolve the virtual address (including direct mapping, ioremap or user space address to
 * its real physical address.
 *
 * @parm vaddr indicates any virtual address
 *
 * @return  >= 0 for success, 0xFFFFFFFF for fail
 **/
phys_addr_t frm_va2pa(u32 vaddr);


/*
 * @This function gets system memory allocation alignment. Currently, the system memory
 *  is managed by FRAMMAP. So this function should be deprecated.
 *
 * @function int frm_get_sysmem_alignment(void)
 * @param: None
 * @return value: 0: PAGE alignment(default value), others for 2^sys_align * 4K
 */
u32 frm_get_sysmem_alignment(void);

/*
 * @This function shows memory distribution. Through this function, we can know how many
 *  memory is avaliable and how many memory is consumed.
 *
 * @function void show_ddr_info(void)
 * @param: None
 * @return value: None.
 */
void show_ddr_info(void);

/*
 * @This function finds the address which is allocated from frammap.
 *
 * @function int frm_find_addrinfo(frammap_buf_t *bufino, int by_phyaddr)
 *
 * @param: input: bufino->phy_addr or bufinfo->vaddr which is kernel address
 *                by_phyaddr 1 if searched by physical address, 0 for by virtual address.
 *         output: bufino
 * @return value: 0 for success, -1 for fail
 */
int frm_get_addrinfo(frammap_buf_t *bufino, int by_phyaddr);

/*
 * @This function is used to drain cpu write buffer while accessing a NCB memory.
 */
static inline void frm_drain_writebuffer(void)
{
    __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 4":: "r"(0));
}

/*
 * @This function is used to read CPU id and pci id
 * @Return value: 0 for success, -1 for fail
 */
typedef enum {
    FRM_CPU_FA726 = 0,
    FRM_CPU_FA626,
    FRM_CPU_UNKNOWN,
} frm_cpu_id_t;

typedef enum {
    FRM_PCI_HOST = 0,
    FRM_PCI_DEV0,
} frm_pci_id_t;

int frm_get_identifier(frm_pci_id_t *pci_id, frm_cpu_id_t *cpu_id);

#endif /* __KERNEL__ */

/*
 * The following structure is used for user space to allocate memory from memory except system.
 */
typedef struct frmmap_meminfo_s
{
	unsigned int    aval_bytes; ///< avaliable bytes of the DDR allows AP to get once
	void            *drv_data;  ///< internal use, please don't touch this pointer
} frmmap_meminfo_t;

#define FRAMMAP_MINOR       100
#define FRM_IOC_MAGIC       'j'
#define FRM_IOGBUFINFO      _IOR(FRM_IOC_MAGIC, 1, struct frmmap_meminfo_s)
#define FRM_GIVE_FREEPAGES  _IOW(FRM_IOC_MAGIC, 2, int)
#define FRM_ALLOC_TYPE      _IOW(FRM_IOC_MAGIC, 3, int)
#define FRM_ADDR_V2P        _IOWR(FRM_IOC_MAGIC, 4, unsigned int)

#endif /* __FRAMMAP_IF_H__ */


