#include <common.h>
#include <asm/io.h>

#if defined(CONFIG_CMD_NAND)
#if !defined(CFG_NAND_LEGACY)

#include <nand.h>
#include <config.h>

//======================================================
int nand_read_buffer(u_char * buf, int len);
int nand_read_page_spare(int startpg, unsigned char *pg_buf, unsigned char *spare_buf, int len);

//======================================================

static NANDCHIP_T gm_nand_chip;

extern BOOL ECC_function(UINT8 u8_channel, UINT8 u8_choice);
extern struct collect_data_buf p_buf;
struct flash_info flash_parm = { 0 };
unsigned char pgbuf[8192];
NAND_SYSHDR *nand_sys_hdr;
extern UINT32 cq4_default;

int gSTATUSReg;
int gSTATUSRegValid;
int command_mode = 0, BI_data = 0, no_system_header = 0, cmdfunc_status = NAND_STATUS_READY;

/* These really don't belong here, as they are specific to the NAND Model */
static uint8_t scan_ff_pattern[] = { 0xff, 0xff };

//=============================================================================
// NAND BI table, size = 1024 bytes
//=============================================================================
typedef struct bi_table {
    /* This array size is related to USB_BUFF_SZ defined in usb_scsi.h */
    unsigned int bi_status[256];        //each bit indicates a block. 1 for good, 0 for bad
} BI_TABLE;

BI_TABLE bi_table;

static struct nand_bbt_descr bbt_descr = {
    .options = 0,
    .offs = 0,
    .len = 1,
    .pattern = scan_ff_pattern
};

static struct nand_ecclayout oob_descr_2048 = {
    .eccbytes = 12,             //including 3 ECC
    .eccpos = {
               8, 9, 10,
               24, 25, 26,
               40, 41, 42,
               56, 57, 58,
               },
    .oobfree = {
                {.offset = 1,.length = 7},
                {.offset = 13,.length = 3},
                {.offset = 17,.length = 7},
                {.offset = 29,.length = 3},
                {.offset = 33,.length = 7},
                {.offset = 45,.length = 3},
                {.offset = 49,.length = 7},
                {.offset = 61,.length = 3},
                }
};

u32 fLib_NANDC_ReadReg(u32 Offset)
{
    u32 val;

    val = readl(FTNANDC023_BASE + Offset);
    return val;
}

void fLib_NANDC_WriteReg(u32 Offset, u32 RegValue)
{
    writel(RegValue, FTNANDC023_BASE + Offset);
}

/*
 * Write buf to the NANDC023 Controller Data Buffer
 */
static void nandc023_write_buf(struct mtd_info *mtd, const u_char * buf, int len)
{
		unsigned char spare_buf[64] = {0xFF};
		 
    NANDC_DEBUG1("%s in, len = %d, buf addr = 0x%x\n", __FUNCTION__, len, buf);

    if (command_mode == NAND_CMD_SEQIN) {
        if (len == p_buf.u32_data_length_in_bytes) {
            p_buf.u8_data_buf = (unsigned char *) buf;
            /* DMA starts */
            Data_write(NAND_CHANNEL, &p_buf, &flash_parm, 1, 0);

            //Command FAILED      
            if (!Check_command_status(NAND_CHANNEL, 1)) {
                printf("MSG: check status fail in ftnandc023_write_buffer");
            }
        } else {
            //write ready when run NAND_CMD_SEQIN
        }
    }else if (command_mode == NAND_CMD_BI_TABLE) {
    		if (len == p_buf.u32_data_length_in_bytes) {//2k body
    			spare_buf[0] = 0;
	    		p_buf.u8_spare_data_buf = (unsigned char *) spare_buf;
	        /* DMA starts */
	        Data_write(NAND_CHANNEL, &p_buf, &flash_parm, 1, 0);
	
	        //Command FAILED      
	        if (!Check_command_status(NAND_CHANNEL, 0)) {
	            printf("MSG: check status fail in ftnandc023_write_buffer");
	        }
        } else {//spare area
            //write ready when run NAND_CMD_SEQIN
        }	        
    }

    NANDC_DEBUG1("%s exit\n", __FUNCTION__);
    return;
}

/*
 * These functions are quite problematic for the NANDC023. Luckily they are
 * not used in the current nand code, except for nand_command, which
 * we've defined our own anyway. The problem is, that we always need
 * to write 4 bytes to the NANDC023 Data Buffer, but in these functions we
 * don't know if to buffer the bytes/half words until we've gathered 4
 * bytes or if to send them straight away.
 *
 * Solution: Don't use these with NANDC023 and complain loudly.
 */
static void nandc023_cmd_ctrl(struct mtd_info *mtd, int dat, unsigned int ctrl)
{
    //printf("%s: WARNING, this function does not implement with the NANDC023!\n",__FUNCTION__);
}

/* The original:
 * static void nandc023_read_buf(struct mtd_info *mtd, const u_char *buf, int len)
 *
 * Shouldn't this be "u_char * const buf" ?
 */
static void nandc023_read_buf(struct mtd_info *mtd, u_char * const buf, int len)
{
    NANDC_DEBUG3("%s in\n", __FUNCTION__);

    //printf("len = %d\n", len);
    nand_read_buffer(buf, len);

    NANDC_DEBUG3("%s exit\n", __FUNCTION__);
    return;
}

/*
 * read a word. Not implemented as not used in NAND code.
 */
static u16 nandc023_read_word(struct mtd_info *mtd)
{
    printf("%s: UNIMPLEMENTED.\n", __FUNCTION__);
    return 0;
}

#define ID_num	4               // ID has n bytes
int read_id_time = 0, ID_point = 0;
unsigned char buf[ID_num] = { 0 };

/*
 * read a byte from DATA port Because we can only read 4 bytes from DATA port at
 * a time, we buffer the remaining bytes. The buffer is reset when a
 * new command is sent to the chip.
 *
 */
static u_char nandc023_read_byte(struct mtd_info *mtd)
{
    NANDC_DEBUG1("%s in\n", __FUNCTION__);
    if (command_mode == NAND_CMD_READID) {
        if (read_id_time == 0) {
            nand_read_buffer(buf, ID_num);
            printf("ID = 0x%x,0x%x,0x%x,0x%x\n", buf[0], buf[1], buf[2], buf[3]);
            read_id_time = 1;
        }
    } else {
        buf[0] = (FTNANDC023_32BIT(NANDC_General_Setting) >> 2) & 1;    //1: write protect
        NANDC_DEBUG1("%s exit\n", __FUNCTION__);
        return buf[0] = 1 ? NAND_STATUS_WP : 0;
    }

    NANDC_DEBUG1("%s exit\n", __FUNCTION__);
    return buf[ID_point++];
}

/* this function is called nandc023 Programm and Erase Operations to
 * check for success or failure */
static int nandc023_wait(struct mtd_info *mtd, struct nand_chip *this)
{
    NANDC_DEBUG1("%s\n", __FUNCTION__);
    return cmdfunc_status;
}

/* Read Page and spare
 */
BOOL ftnandc023_read_page_with_spare(unsigned int u32_row_addr,
                                     unsigned char u8_sector_offset,
                                     UINT16 u16_sector_cnt, struct collect_data_buf * data_buf)
{
    struct command_queue_feature cmd_f;

    /* Spare Length check
     */
    if (data_buf->u8_mode == Sector_mode) {
        if (data_buf->u16_spare_length != (u16_sector_cnt * data_buf->u8_spare_size_for_sector)) {
            NANDC_DEBUG1("MSG: f1");
            return 0;
        }
    } else if (data_buf->u8_mode == Page_mode) {
        /* do nothing, fall through */
    } else
        return 0;

    if (data_buf->u16_spare_length == 0) {
        NANDC_DEBUG1("MSG: f2");
        return 0;
    }
    if (data_buf->u8_spare_data_buf == NULL) {
        NANDC_DEBUG1("MSG: f3");
        return 0;
    }

    /* Data Length check
     */
    if (data_buf->u32_data_length_in_bytes != (u16_sector_cnt * flash_parm.u16_sector_size)) {
        NANDC_DEBUG1("MSG: f4");
        return 0;
    }
    if (data_buf->u32_data_length_in_bytes == 0) {
        NANDC_DEBUG1("MSG: f5");
        return 0;
    }
    if (data_buf->u8_data_buf == NULL) {
        NANDC_DEBUG1("MSG: f6");
        return 0;
    }

    /* byte mode is disabled */
    data_buf->u8_bytemode = Byte_Mode_Disable;
    data_buf->u16_data_length_in_sector = u16_sector_cnt;       //how many sectors

    init_cmd_feature_default(&cmd_f);

    //Clear spare area to ensure read spare data is not previous written value.
    FTNANDC023_clear_specified_spare_sram(NAND_CHANNEL, data_buf);

    // Checking the command queue isn't full
    Command_queue_status_full(NAND_CHANNEL);

    Page_read_with_spare_setting(NAND_CHANNEL, u32_row_addr, u8_sector_offset, data_buf);
    //Setting_feature_and_fire(NAND_CHANNEL, Command(PAGE_READ_WITH_SPARE), Start_From_CE0, *u32_sixth_word);
    Setting_feature_and_fire(NAND_CHANNEL, &cmd_f);

    return 1;
}

void Block_erase_setting(unsigned char u8_channel, unsigned int u32_row_addr, UINT16 u16_block_cnt)
{
    struct command_queue_feature cmd_f;

	write_protect(0);

	cmd_f.row_cycle = Row_address_2cycle;
	cmd_f.col_cycle = Column_address_2cycle;
	
	cmd_f.cq1 = (u32_row_addr & 0xFFFFFF);
	cmd_f.cq2 = 0;
	cmd_f.cq3 = (u16_block_cnt << 16);
	cmd_f.cq4 = cq4_default | CMD_COMPLETE_EN |	CMD_SCALE(1) | CMD_INDEX(ERASE);
			
    Setting_feature_and_fire(u8_channel, &cmd_f);
}

/* Erase blocks
 */
BOOL ftnandc023_erase_block(UINT16 u16_block_id, UINT16 u16_block_cnt,
                            struct flash_info * flash_parm)
{
    unsigned int u32_row_addr1;

    u32_row_addr1 = u16_block_id * flash_parm->u16_page_in_block;       //which page
    NANDC_DEBUG1("page num = %d\n", u32_row_addr1);

    Block_erase_setting(NAND_CHANNEL, u32_row_addr1, u16_block_cnt);

    return 1;
}

static void ftnandc023_readid(unsigned char channel)
{
    struct command_queue_feature cmd_f;
    unsigned int id_size = 4;

	cmd_f.row_cycle = Row_address_1cycle;
	cmd_f.col_cycle = Column_address_1cycle;
	cmd_f.cq1 = 0;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = CMD_COUNT(1);
	cmd_f.cq4 = cq4_default | CMD_COMPLETE_EN |	CMD_INDEX(READ_ID) |\
			CMD_BYTE_MODE | CMD_SPARE_NUM(id_size);
	
	Setting_feature_and_fire(channel, &cmd_f);

    NANDC_DEBUG1("%s exit\n", __FUNCTION__);
}

/* Reset the nand flash, not NAND
 */
void ftnandc023_reset_flash(unsigned char channel)
{
    struct command_queue_feature cmd_f;

	cmd_f.cq1 = 0;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = 0;
	cmd_f.cq4 = cq4_default | CMD_COMPLETE_EN | CMD_INDEX(RESET);

    Setting_feature_and_fire(channel, &cmd_f);
}

/* 0 is good, 1 is bad */
int ftnandc_read_bbt(int block_num)
{
		int data, ret;
		
    data = bi_table.bi_status[block_num / 32];
    data = (data >> (block_num % 32)) & 0x1;
    if(data)
    	ret = 0;
    else
    	ret = 1;
    
    return ret;
}

void ftnandc_set_bbt(int block_num)
{
		int data;
		
    data = bi_table.bi_status[block_num / 32];
    data &= ~(1 << (block_num % 32));
    bi_table.bi_status[block_num / 32] = data;
}

/* cmdfunc send commands to the NANDC023 */
static void nandc023_cmdfunc(struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
    /* register struct nand_chip *this = mtd->priv; */
    unsigned long block_num = 0;

    NANDC_DEBUG1("nandc023_cmdfunc=0x%x\n", command);
    switch (command) {
    case NAND_CMD_READ0:
        NANDC_DEBUG1
            ("nandc023_cmdfunc: NAND_CMD_READ0, page_addr: 0x%x, column: 0x%x.\n",
             page_addr, column);

        command_mode = NAND_CMD_READ0;
        nand023_read_page(page_addr);
        goto end;

    case NAND_CMD_READ1:
        printf("nandc023_cmdfunc: NAND_CMD_READ1 unimplemented!\n");
        goto end;

    case NAND_CMD_READOOB:
        NANDC_DEBUG1
            ("nandc023_cmdfunc: NAND_CMD_READOOB, page_addr: 0x%x, column: 0x%x.\n",
             page_addr, (column >> 1));

        command_mode = NAND_CMD_READOOB;

        // calc block number, mtd->erasesize / mtd->writesize =  one block n pages
        block_num = page_addr / (mtd->erasesize / mtd->writesize);
#ifdef CONFIG_CMD_FPGA
        BI_data = 1;
#else
        BI_data = ftnandc_read_bbt(block_num);        
#endif
        goto end;

    case NAND_CMD_READID:
        NANDC_DEBUG1("nandc023_cmdfunc: NAND_CMD_READID.\n");
        command_mode = NAND_CMD_READID;
        ftnandc023_readid(NAND_CHANNEL);

        goto end;

    case NAND_CMD_PAGEPROG:
        /* sent as a multicommand in NAND_CMD_SEQIN */
        NANDC_DEBUG1("nandc023_cmdfunc: NAND_CMD_PAGEPROG not need to implement.\n");
        goto end;

    case NAND_CMD_ERASE1:
        NANDC_DEBUG1("nandc023_cmdfunc: NAND_CMD_ERASE1, page_addr: 0x%x.\n", page_addr);
        ftnandc023_erase_block(page_addr / flash_parm.u16_page_in_block, 1, &flash_parm);
            
        goto Check_status;

    case NAND_CMD_ERASE2:
        NANDC_DEBUG1("nandc023_cmdfunc: NAND_CMD_ERASE2 not need to implement.\n");
        goto end;

    case NAND_CMD_SEQIN:
        /* send PAGE_PROG command(0x1080) */

        NANDC_DEBUG1
            ("nandc023_cmdfunc: NAND_CMD_SEQIN/PAGE_PROG,  page_addr: 0x%x, column: 0x%x.\n",
             page_addr, (column >> 1));

        command_mode = NAND_CMD_SEQIN;
        nand023_write_page(page_addr);
        goto end;

    case NAND_CMD_BI_TABLE:
        /* send PAGE_PROG command(0x1080) */
        NANDC_DEBUG1
            ("nandc023_cmdfunc: NAND_CMD_SEQIN/PAGE_PROG,  page_addr: 0x%x, column: 0x%x.\n",
             page_addr, (column >> 1));

        command_mode = NAND_CMD_BI_TABLE;
        nand023_write_page(page_addr);
        goto end;
        
    case NAND_CMD_STATUS:
        NANDC_DEBUG1("nandc023_cmdfunc: NAND_CMD_STATUS.\n");
        goto end;

    case NAND_CMD_RESET:
        NANDC_DEBUG1("nandc023_cmdfunc: NAND_CMD_RESET.\n");
        ftnandc023_reset_flash(NAND_CHANNEL);
        goto Check_status;

    default:
        printf("nandc023_cmdfunc: error, unsupported command (0x%x).\n", command);
        goto end;
    }

  Check_status:

    /*  wait_event: */
    if (!Check_command_status(NAND_CHANNEL, 1)) {
        printf("Check_command_status fail");
        cmdfunc_status = NAND_STATUS_FAIL;
        return;
    } else {
        cmdfunc_status = NAND_STATUS_READY;
    }
  end:
    return;
}

/**
 * nand_verify_buf32 - [DEFAULT] Verify chip data against buffer
 * @mtd:	MTD device structure
 * @buf:	buffer containing the data to compare
 * @len:	number of bytes to compare
 *
 * Default verify function for 32bit buswith
 */
#define	EFAULT 14
static int nand_verify_buf32(struct mtd_info *mtd, const u_char * buf, int len)
{
    printf("%s\n", __FUNCTION__);
    printf("buf=0x%x,len=%d\n", (unsigned int)buf, (unsigned int)len);

    return 0;
}

static void hwctl(struct mtd_info *mtd, int mode)
{
    //hw do it, not need implement
    NANDC_DEBUG3("%s in\n", __FUNCTION__);
}

static int calculate(struct mtd_info *mtd, const uint8_t * dat, uint8_t * ecc_code)
{
    //hw do it, not need implement
    NANDC_DEBUG3("%s in\n", __FUNCTION__);
    return 0;
}

static int correct(struct mtd_info *mtd, uint8_t * dat, uint8_t * read_ecc, uint8_t * calc_ecc)
{
#if CONFIG_NAND_NO_OOBR || CONFIG_NAND_NO_OOBW
    return 0;
#else
    NANDC_DEBUG3("%s\n", __FUNCTION__);
    return 0;
#endif
}

NANDCHIP_T *nand023_init(unsigned int *bootm_addr)
{
    struct collect_data_buf data_buf;
    char spare_buf[SPARE_SIZE_OF_PAGE];

    int startpg;

    nand023_drv_init();
    nand_load_default(&gm_nand_chip);

    nand023_drv_load_config(gm_nand_chip.blocksize / gm_nand_chip.pagesize,
                            gm_nand_chip.pagesize,
                            gm_nand_chip.sparesize,
                            gm_nand_chip.status_bit,
                            gm_nand_chip.cmmand_bit,
                            gm_nand_chip.ecc_capability,
                            gm_nand_chip.ecc_base, gm_nand_chip.row_cyc, gm_nand_chip.col_cyc);

    printf("ECC is %d bits per sector. \n", gm_nand_chip.ecc_capability);
    /* read the system header */
    memset(&data_buf, 0, sizeof(struct collect_data_buf));
    data_buf.u8_data_buf = (unsigned char *) pgbuf;
    data_buf.u16_data_length_in_sector = 1;     //read one sector for system header
    data_buf.u32_data_length_in_bytes =
        data_buf.u16_data_length_in_sector * gm_nand_chip.sectorsize;
    /* prepare spare buf */
    data_buf.u8_spare_data_buf = (unsigned char *) spare_buf;
    data_buf.u8_spare_size_for_sector = USR_SPARE_SIZE;
    data_buf.u16_spare_length =
    data_buf.u8_spare_size_for_sector * data_buf.u16_data_length_in_sector;
    
#ifdef CONFIG_GM_AHBDMA
    printf("DMA mode\n");
#endif
    startpg = 0;
    NANDC_DEBUG1("Read system header\n");
    
    /* may cause ECC error due to all 0xFF */
    if (!nand023_drv_read_page_with_spare(startpg, 0, 1, &data_buf, 0)) {
        printf("NAND readpage fail!\n");
        return NULL;
    }

    /* check signature */
    if (memcmp((char *)pgbuf, SS_SIGNATURE, 0x6) != 0) {
    		no_system_header = 1;
        printf("NAND: error system header!\n");
        //return NULL;
    }
    // check 0xAA55_Signature
    if ((((INT8U *) pgbuf)[0x1FE] != 0x55) || (((INT8U *) pgbuf)[0x1FF] != 0xAA)) {
    		no_system_header = 1;
        printf("NAND: System Header 0x55AA check fail!\n");
        //return NULL;
    }
    
    nand_sys_hdr = (NAND_SYSHDR *) pgbuf;
    *bootm_addr = nand_sys_hdr->bootm_addr;

    return &gm_nand_chip;
}

int nand_read_buffer(u_char * buf, int len)
{
    int ret;
    int i;

    if (command_mode == NAND_CMD_READOOB) {
        if (BI_data) {
            for (i = 0; i < len; i++)
                buf[i] = 0xFF;
        } else {
            for (i = 0; i < len; i++)
                buf[i] = 0;
        }
    }

    if (command_mode == NAND_CMD_READ0) {
        if (len == p_buf.u32_data_length_in_bytes) {
            p_buf.u8_data_buf = (unsigned char *) buf;
            NANDC_DEBUG1("nand_read_buffer data len = %d, buf addr = 0x%x\n", len, buf);
            //nand_read_page_spare(1, buf, buf, len);                       

            /* mode */
            //data_buf.u8_mode = Sector_mode;
            //data_buf.u8_bytemode = Byte_Mode_Disable;   

            /* DMA starts */
            //printf("read data\n");
            Data_read(NAND_CHANNEL, &p_buf, &flash_parm, 1, 0); //read data page

        } else {
            NANDC_DEBUG1("nand_read_buffer spare len = %d, buf addr = 0x%x\n", len, buf);
            p_buf.u8_spare_data_buf = (unsigned char *) buf;

            //printf("read spare\n");
            Data_read(NAND_CHANNEL, &p_buf, &flash_parm, 0, 1); //read spare
            
            //Command FAILED      
            ret =
                Check_command_status(NAND_CHANNEL,
                                     spare_dirty(p_buf.u8_spare_size_for_sector, buf));
            if (!ret) {
                printf("MSG: check status fail in ftnandc023_read_buffer");
                return 0;
            }
        }
    }

    if (command_mode == NAND_CMD_READID) {
        NANDC_DEBUG1("get ID\n");
        //ftnandc023_readid(NAND_CHANNEL);
        p_buf.u8_spare_data_buf = (unsigned char *) buf;

        //Command FAILED      
        ret =
            Check_command_status(NAND_CHANNEL,
                                 spare_dirty(p_buf.u8_spare_size_for_sector, buf));
        if (!ret) {
            printf("MSG: check status fail in ftnandc023_read_buffer");
            return 0;
        }
        /* DMA starts */
        //Data_read(NAND_CHANNEL, &p_buf, &flash_parm, 0, 1);  //read data page
        *(unsigned int *)buf = FTNANDC023_32BIT(Spare_Sram_Access);
    }
    
    return 1;
}

/*
 * pg_buf: the page buffer
 * spare_buf: the spare buffer
 * len: how many bytes want to read
 * Return Value:
 *      >= 0 for success, < 0 for fail
 */
int nand_read_page_spare(int startpg, unsigned char *pg_buf, unsigned char *spare_buf, int len)
{
    struct collect_data_buf data_buf;
    int count;

    NANDC_DEBUG1("nand_read_page_spare, startpg = 0x%x, len = 0x%x\n", startpg, len);
        
    memset(&data_buf, 0, sizeof(struct collect_data_buf));
    data_buf.u8_data_buf = (unsigned char *) pg_buf;
    //how many sectors
    count = data_buf.u16_data_length_in_sector =
        (len + (gm_nand_chip.sectorsize - 1)) / gm_nand_chip.sectorsize;
    //translate to real length from sector
    data_buf.u32_data_length_in_bytes =
        data_buf.u16_data_length_in_sector * gm_nand_chip.sectorsize;
    /* prepare spare buf */
    data_buf.u8_spare_data_buf = (unsigned char *) spare_buf;
    data_buf.u8_spare_size_for_sector = USR_SPARE_SIZE;
    data_buf.u16_spare_length =
        data_buf.u8_spare_size_for_sector * data_buf.u16_data_length_in_sector;
    /* may cause ECC error due to all 0xFF */
    if (!nand023_drv_read_page_with_spare(startpg, 0, count, &data_buf, 1)) {
        printf("NAND:nand_read_page_spare fail!\n");
        return -1;
    }
    NANDC_DEBUG1("%s exit\n", __FUNCTION__);
    return 1;
}

/*
 * Board-specific NAND initialization. The following members of the
 * argument are board-specific (per include/linux/mtd/nand_new.h):
 * - IO_ADDR_R?: address to read the 8 I/O lines of the flash device
 * - IO_ADDR_W?: address to write the 8 I/O lines of the flash device
 * - hwcontrol: hardwarespecific function for accesing control-lines
 * - dev_ready: hardwarespecific function for  accesing device ready/busy line
 * - enable_hwecc?: function to enable (reset)  hardware ecc generator. Must
 *   only be provided if a hardware ECC is available
 * - eccmode: mode of ecc, see defines
 * - chip_delay: chip dependent delay for transfering data from array to
 *   read regs (tR)
 * - options: various chip options. They can partly be set to inform
 *   nand_scan about special functionality. See the defines for further
 *   explanation
 * Members with a "?" were not set in the merged testing-NAND branch,
 * so they are not set here either.
 */
int board_nand_init(struct nand_chip *nand)
{
    unsigned int image_offset;
    unsigned char spare_buf[64];
    int start_pg, value;

    NANDCHIP_T *init_nand_chip;

    /* DMA init */
    Dma_init((1 << REQ_NANDRX) | (1 << REQ_NANDTX));

    init_nand_chip = nand023_init(&image_offset);
    if (!init_nand_chip) {
        printf("NAND boots fail! \n");
        return -1;
    }

    /* Load BI TABLE */
    memset(&bi_table, 0, sizeof(BI_TABLE));
    NANDC_DEBUG1("read BI table!");
		if(no_system_header){
				printf("========>Not find system header, default assign all blocks are good\n");
				memset(&bi_table, 0xFF, sizeof(BI_TABLE));
		}else{
		    start_pg = (sizeof(NAND_SYSHDR) + init_nand_chip->pagesize - 1) / init_nand_chip->pagesize;

    		value = FTNANDC023_8BIT(ECC_Control + 1);
    		ECC_function(NAND_CHANNEL, 0);

		    if (nand_read_page_spare(start_pg, (unsigned char *)&bi_table, spare_buf, init_nand_chip->sectorsize) < 0)  //read one sector
		    {
		        printf("NAND: Fail to read BI table!");
		        return -1;
		    }
		    
    		if(value)
    				ECC_function(NAND_CHANNEL, 1);		    
		}
    NANDC_DEBUG1("NAND: read BI table finished!");

    nand->waitfunc = nandc023_wait;
    nand->read_byte = nandc023_read_byte;
    nand->read_word = nandc023_read_word;
    nand->read_buf = nandc023_read_buf;
    nand->write_buf = nandc023_write_buf;

    nand->cmdfunc = nandc023_cmdfunc;

    /* our HW auto check ECC, not need to read field and compare again */
    nand->ecc.mode = NAND_ECC_NONE;     //NAND_ECC_HW;       
    nand->ecc.calculate = calculate;
    nand->ecc.correct = correct;
    nand->ecc.hwctl = hwctl;

    nand->ecc.size = (init_nand_chip->ecc_base == 1) ? 1024 : 512;
    nand->ecc.bytes = (init_nand_chip->ecc_capability * 4) / 8;
    nand->ecc.layout = &oob_descr_2048;

    NANDC_DEBUG1("ECC = %d, %d\n", nand->ecc.size, nand->ecc.bytes);

    nand->badblock_pattern = &bbt_descr;        //at start, u-boot try to scan all block with this pattern
    nand->verify_buf = nand_verify_buf32;
    nand->cmd_ctrl = nandc023_cmd_ctrl;

    return 0;
}
#else
#error "U-Boot legacy NAND support not available for NANDC023."
#endif
#endif

//nand_get_flash_type:mtd->writesize=0x800,mtd->oobsize=0x40,mtd->erasesize=0x20000
