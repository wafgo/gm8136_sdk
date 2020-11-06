#ifndef __NAND_H__
#define __NAND_H__

//#define _NAND_DEBUG_		1

#ifdef 	_NAND_DEBUG_
#define	NAND_DEBUG(format, args...)	KPrint(format, ##args)
#else
#define NAND_DEBUG(x)
#endif

#define IMAGE_MAGIC     0x805A474D
#define SS_SIGNATURE    "GM8136"

//=============================================================================
// NAND System Header, size = 512 bytes
//=============================================================================
typedef struct sys_header 
{
    char signature[8];          /* Signature is "GM8xxx" */
    unsigned int bootm_addr;    /* Image offset to load by spiboot */
    unsigned int bootm_size;
    unsigned int bootm_reserved;
    
    struct {
        unsigned int addr;          /* image address */
        unsigned int size;          /* image size */
        unsigned char name[8];      /* image name */
        unsigned int reserved[1];
    } image[10];
    
    struct {
        unsigned int nand_numblks;      //number of blocks in chip
        unsigned int nand_numpgs_blk;   //how many pages in a block
        unsigned int nand_pagesz;       //real size in bytes                        
        unsigned int nand_sparesz_inpage;       //64bytes for 2k, ...... needed for NANDC023
        unsigned int nand_numce;        //how many CE in chip
        unsigned int nand_status_bit;
        unsigned int nand_cmmand_bit;
        unsigned int nand_ecc_capability;
        unsigned int nand_ecc_base;     //0/1 indicates 512/1024 bytes      
        unsigned int nand_sparesz_insect;
        unsigned int reserved_1;
        unsigned int nand_row_cycle;    //1 for 1 cycle ...
        unsigned int nand_col_cycle;    //1 for 1 cycle ...
        unsigned int reserved[1];
    } nandfixup;
    unsigned int reserved2[58];        // unused
    unsigned char last_511[4];  // byte510:0x55, byte511:0xAA
} NAND_SYSHDR;

//=============================================================================
// NAND BI table, size = 1024 bytes
//=============================================================================
typedef struct bi_table
{
    /* This array size is related to USB_BUFF_SZ defined in usb_scsi.h */
    unsigned int    bi_status[256]; //each bit indicates a block. 1 for good, 0 for bad
} BI_TABLE;

typedef struct _nandchip 
{
    int sectorsize;
	int pagesize;
	int blocksize;
	int chipsize;
	int sparesize;
	int ecc_base;       //0:512, 1:1024	
	int ecc_capability; //0 ~ 23 which indicates 1 ~ 24 bits.
	int status_bit;
	int cmmand_bit;
	int row_cyc;
	int col_cyc;
} NANDCHIP_T;

extern NANDCHIP_T *nand_init(unsigned int *bootm_addr, unsigned int *bootm_size);
/*
 * pg_buf: the page buffer
 * spare_buf: the spare buffer
 * len: how many bytes want to read
 * Return Value:
 *      >= 0 for success, < 0 for fail
 */
extern int nand_read_page_spare(int startpg, unsigned char *pg_buf, unsigned char *spare_buf, int len);
       
/* -1 for search fail, others for success to find good block number */
extern int nand_get_next_good_block(UINT32 cur_blk, BI_TABLE *bi_table);

#endif
