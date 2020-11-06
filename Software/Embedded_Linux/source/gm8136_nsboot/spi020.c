#include <stdio.h>
#include <string.h>
#include "spi020.h"
#include "ahb_dma.h"
#include "kprint.h"

/* ---------------------------------------------------------------------------------------
 * Global structures
 * ---------------------------------------------------------------------------------------
 */
extern FLASH_TYPE_T flash_type;

struct spi_flash  flash_info;
int spi_init_done = 0;
/* ---------------------------------------------------------------------------------------
 * External functions
 * ---------------------------------------------------------------------------------------
 */
extern void ftspi020_setup_flash(struct spi_flash *flash);
extern int flash_set_quad_enable(struct spi_flash *flash, int chip_id);
extern int spi_dataflash_en4b(struct spi_flash *flash, int enable);
/* ---------------------------------------------------------------------------------------
 * Local functions
 * ---------------------------------------------------------------------------------------
 */
/* ---------------------------------------------------------------------------------------
 * Internal functions
 * ---------------------------------------------------------------------------------------
 */
BOOL ftspi020_Start_DMA(unsigned int Channel,	// use which channel for AHB DMA, 0..7
		      unsigned int SrcAddr,	// source begin address
		      unsigned int DstAddr,	// dest begin address
		      unsigned int Size,	    // total bytes
		      unsigned int SrcWidth,	// source width 8/16/32 bits -> 0/1/2
		      unsigned int DstWidth,	// dest width 8/16/32 bits -> 0/1/2
		      unsigned int SrcSize,	// source burst size, How many "SrcWidth" will be transmmited at one times ?
		      unsigned int SrcCtrl,	// source address change : Inc/dec/fixed --> 0/1/2
		      unsigned int DstCtrl,	// dest address change : Inc/dec/fixed --> 0/1/2
		      unsigned int Priority,	// priority for this chaanel 0(low)/1/2/3(high)
		      unsigned int Mode)	    // Normal/Hardwire,   0/1
{
    unsigned int direction;
    int dma_ch;
    int remain;

    //KPrint("%s in\n", __FUNCTION__);
    direction = (SrcCtrl == 0) ? DMA_MEM2SPI020 : DMA_SPI0202MEM;

    do {
        remain = Size > 3 * 1024 * 1024 ? 3 * 1024 * 1024 : Size;

        dma_ch = Dma_go_trigger((unsigned int *) SrcAddr, (unsigned int *) DstAddr, remain, NULL, direction, 1);
        if (dma_ch >= 0) {
            Dma_wait_timeout(dma_ch, 0);
        } else {
            KPrint("%s exit 0\n", __FUNCTION__);
            return 0;
        }

        Size -= remain;

        if (SrcCtrl == 0)       /* increase */
            SrcAddr += remain;
        if (DstCtrl == 0)
            DstAddr += remain;
    } while (Size > 0);

    //KPrint("%s exit 1\n", __FUNCTION__);
    return 1;
}

void ftspi020_scu_init(void)
{
    //KPrint("scu init must be set...\n");
    //issue RESET 
    return;
}

/* SPI Read Status Register. It responds the status from FLASH.
 */
void ftspi020_read_status(UINT8 * status)
{
	*status = FTSPI020_8BIT(SPI_READ_STATUS);
}

/* Reset SPI020 controller.
 */
void ftspi020_reset_hw(void)
{
	FTSPI020_32BIT(CONTROL_REG) |= BIT8;
}

/* Set spi_clk_mode while IDLE state.
 * 0: mode0, sck_out will be low at the IDLE state
 * 1: mode3, sck_out will be high at the IDLE state
 */
void ftspi020_operate_mode(UINT8 mode)
{
	FTSPI020_32BIT(CONTROL_REG) &= ~BIT4;
	FTSPI020_32BIT(CONTROL_REG) |= ((mode << 4) & BIT4);
}

void ftspi020_busy_location(UINT8 location)
{
	FTSPI020_32BIT(CONTROL_REG) &= ~(BIT18 | BIT17 | BIT16);
	FTSPI020_32BIT(CONTROL_REG) |= ((location << 16) & (BIT18 | BIT17 | BIT16));
}

void ftspi020_divider(UINT8 divider)
{
	UINT8 val;
        
    if (divider == 0)   //do nothing
        return;
        
	if (divider == 2)
		val = divider_2;
	else if (divider == 4)
		val = divider_4;
	else if (divider == 6)
		val = divider_6;
	else if (divider == 8)
		val = divider_8;
	else {
		KPrint("Not valid divider value %d\n", divider);
		return ;
	}

	FTSPI020_32BIT(CONTROL_REG) &= ~(BIT1 | BIT0);
	FTSPI020_32BIT(CONTROL_REG) |= ((val) & (BIT1 | BIT0));
}

/* polling command complete interrupt until the interrupt comes.
 */
void ftspi020_wait_cmd_complete(void)
{
    volatile unsigned int value;
    
    do {
        value = FTSPI020_32BIT(INTR_STATUS_REG);
        if (value & cmd_complete)
            break;
    } while (1);
    FTSPI020_32BIT(INTR_STATUS_REG) |= 0x1;  //write 1 clear 
}

UINT8 ftspi020_issue_cmd(struct ftspi020_cmd *command)
{
	unsigned int cmd_feature1, cmd_feature2;
	
	FTSPI020_32BIT(SPI_FLASH_ADDR) = command->spi_addr;

	cmd_feature1 = ((command->conti_read_mode_en & 0x1) << 28) |
	    ((command->ins_len & 0x3) << 24) | ((command->dum_2nd_cyc & 0xFF) << 16) | ((command->addr_len & 0x7) << 0);
	FTSPI020_32BIT(SPI_CMD_FEATURE1) = cmd_feature1;

	FTSPI020_32BIT(SPI_DATA_CNT) = command->data_cnt;

	cmd_feature2 = ((command->ins_code & 0xFF) << 24) |
	    ((command->conti_read_mode_code & 0xFF) << 16) |
	    ((command->start_ce & 0x3) << 8) |
	    ((command->spi_mode & 0x7) << 5) |
	    ((command->dtr_mode & 0x1) << 4) |
	    ((command->read_status & 0x1) << 3) |
	    ((command->read_status_en & 0x1) << 2) | ((command->write_en & 0x1) << 1) | ((command->intr_en & 0x1) << 0);

	FTSPI020_32BIT(SPI_CMD_FEATURE2) = cmd_feature2;
	
	return 0;
}

/*
 * setup command queue.
 */
void ftspi020_setup_cmdq(ftspi020_cmd_t *spi_cmd, int ins_code, int ins_len, int write_en, int spi_mode, int data_cnt)
{
    memset(spi_cmd, 0x0, sizeof(ftspi020_cmd_t));
    
    spi_cmd->start_ce = start_ce0;   //only one CE
	spi_cmd->ins_code = ins_code;
	spi_cmd->intr_en = cmd_complete_intr_enable;
	spi_cmd->ins_len = ins_len;
	spi_cmd->write_en = write_en;   //0 for the read ata or read status, others are 1.
	spi_cmd->dtr_mode = dtr_disable;
	spi_cmd->spi_mode = spi_mode;
	spi_cmd->data_cnt = data_cnt;

    if (flash_type == FLASH_SPI_NAND) {
        switch (ins_code) {
        case CMD_READ_ID:
            spi_cmd->spi_addr = 0x00;
            spi_cmd->addr_len = addr_1byte;
            break;
        case COMMAND_SET_FEATURES:
            spi_cmd->spi_addr = 0xA0;   // to disble write protect
            spi_cmd->addr_len = addr_1byte;
            break;
        default:
            break;
        }
    }	
	/* update to the controller */
	ftspi020_issue_cmd(spi_cmd);
}

#ifndef CONFIG_SPI_DMA
void ftspi020_wait_rxfifo_ready(void)
{
    unsigned int value, loop = 0x10000;

    do {
    		//udelay (1);
        value = FTSPI020_32BIT(STATUS_REG);
        if (value & (1 << rx_ready))
            break;
    } while (loop--);
    
    if(loop == 0)
        KPrint("spi wait rxfifo time out\n");
}
#endif

void ftspi020_data_access(UINT8 *dout, UINT8 *din, unsigned int len)
{
    //KPrint("in = 0x%x, out = 0x%x, len = 0x%x\n", dout, din, len);
    if (dout != NULL) {
        /* write direction */
#ifdef CONFIG_SPI_DMA        
        ftspi020_Start_DMA(FTSPI020_DMA_TX_CH, 
                        (unsigned int) dout,          /* source */
					    (unsigned int) (REG_SPI020_BASE + SPI020_DATA_PORT),/* destination */ 
					    len,    /* total bytes */
					    0x00,   /* source width 8/16/32 bits -> 0/1/2 */
					    0x00,   /* dest width 8/16/32 bits -> 0/1/2 */
					    4,      /* source burst size, How many "SrcWidth" will be transmmited at one times ? */
					    0,      /* source address change : Inc/dec/fixed --> 0/1/2 */
					    2,      /* dest address change : Inc/dec/fixed --> 0/1/2 */
					    0,      /* priority for this chaanel 0(low)/1/2/3(high) */
					    HW_HANDSHAKE_MODE);     // Normal/Hardwire,   0/1
#else
        KPrint("Not implement write\n");
#endif					    
    }   
    else {//if (din != NULL) {// will read to addr 0
        /* read direction */
#ifdef CONFIG_SPI_DMA
	    ftspi020_Start_DMA(FTSPI020_DMA_RX_CH, 
	                (unsigned int) (REG_SPI020_BASE + SPI020_DATA_PORT),    /* source */
					(unsigned int) din,   /* destination */ 
					len,            /* total bytes */ 
					0x00,           /* source width 8/16/32 bits -> 0/1/2 */
					0x00,           /* dest width 8/16/32 bits -> 0/1/2 */
					4,              /* source burst size, How many "SrcWidth" will be transmmited at one times ? */
					2,              /* source address change : Inc/dec/fixed --> 0/1/2 */
					0,              /* dest address change : Inc/dec/fixed --> 0/1/2 */
					0,              /* priority for this chaanel 0(low)/1/2/3(high) */
					HW_HANDSHAKE_MODE); // Normal/Hardwire,   0/1
#else
        int i, j, loop, rxfifo_depth;
        unsigned int *tmp;			
        
        rxfifo_depth = (FTSPI020_32BIT(FEATURE) >> 8) & 0xFF;
        tmp = (unsigned int *)din;
        if(len > (rxfifo_depth * 4)) {
            loop = len / (rxfifo_depth * 4);
        	for(i = 0; i < loop; i ++){
        		ftspi020_wait_rxfifo_ready();
        		for(j = 0; j < rxfifo_depth; j ++)
        			*tmp++ = FTSPI020_32BIT(SPI020_DATA_PORT);
        	}
        	len = len - (loop * rxfifo_depth * 4);
        }
        if(len > 0) {
            ftspi020_wait_rxfifo_ready();
        	for(j = 0; j < (len / 4); j ++)
        		*tmp++ = FTSPI020_32BIT(SPI020_DATA_PORT);			    
        }
#endif
    }
}

/* Entry point. Retrun 0 for success. < 0 for fail
 */
struct spi_flash *FTSPI020_Init(void)
{
	UINT8 idcode[4];
	ftspi020_cmd_t  spi_cmd;
	
	if (spi_init_done)
	    return 0;
			    
	//KPrint("SPI020 Revision:0x%x\n", FTSPI020_32BIT(REVISION));
	
	/* init the pin mux */
	ftspi020_scu_init();
	
	if (1)  /* init spi020 */
	{
    	/* reset spi020 */
    	ftspi020_reset_hw();

    	/* set mode0 or mode3 for the CLK */
    	ftspi020_operate_mode(mode3);
    	//ftspi020_divider(8);    /* romCode has set it */
    	ftspi020_busy_location(BUSY_BIT0);
    }

    if (flash_type == FLASH_SPI_NAND) {
        /* reset device first */
        FTSPI020_8BIT(INTR_CONTROL_REG) = 0x0;//disable DMA enable

        memset(&spi_cmd, 0, sizeof(ftspi020_cmd_t));
        ftspi020_setup_cmdq(&spi_cmd, CMD_RESET, instr_1byte, spi_write, spi_operate_serial_mode, 0);
        ftspi020_wait_cmd_complete();

        /* disable write protect */
        memset(&spi_cmd, 0, sizeof(ftspi020_cmd_t));
        ftspi020_setup_cmdq(&spi_cmd, COMMAND_SET_FEATURES, instr_1byte, spi_write, spi_operate_serial_mode, 1);
        FTSPI020_32BIT(SPI020_DATA_PORT) = 0;
        ftspi020_wait_cmd_complete();
    }
    
#ifdef CONFIG_SPI_DMA
    /* dma is enable */
    FTSPI020_8BIT(INTR_CONTROL_REG) = 0x1;
#endif

	/* read flash id first */
	memset(idcode, 0, sizeof(idcode));
	/* 11.2.35 Read JEDEC ID(9Fh) */
	memset(&spi_cmd, 0, sizeof(ftspi020_cmd_t));
	ftspi020_setup_cmdq(&spi_cmd, CMD_READ_ID, instr_1byte, spi_read, spi_operate_serial_mode, 3);
	/* read data ID back */
	ftspi020_data_access(NULL, idcode, sizeof(idcode));
	ftspi020_wait_cmd_complete();
	KPrint("SPI %s ID code:0x%02x 0x%02x 0x%02x\n", (flash_type == FLASH_SPI_NAND) ? "NAND" : "NOR", idcode[0], idcode[1], idcode[2]);
	
	ftspi020_setup_flash(&flash_info);
	
	/* set quad mode */
	//flash_set_quad_enable(&flash_info, idcode[0]);
	
	spi_init_done = 1;
	
	return &flash_info;
}

/* Flash Read function, 
 * start_pg: start page
 * len: how many bytes
 * 0 for success, otherwise for fail
 */
int FTSPI020_PgRd(unsigned int start_pg, INT8U *rx_buf, unsigned int len)
{
    int ret, offset;

    if (!spi_init_done)
        return -1;
    
		if (flash_type == FLASH_SPI_NAND)
    		offset = start_pg * flash_info.nand_parm.page_size;
    else
    		offset = start_pg * flash_info.page_size;
     
    ret = flash_info.read(&flash_info, COMMAND_FAST_READ, offset, len, rx_buf);
    //ret = flash_info.read(&flash_info, COMMAND_FAST_READ_QUAD_IO, offset, len, rx_buf);
    
    if (ret)
        return -1;
    
    return 0;
}

/* Flash Write function, 
 * start_pg: start page
 * 0 for success, otherwise for fail
 */
int FTSPI020_PgWr(unsigned int start_pg, INT8U *tx_buf)
{
    int ret, offset, len;
    
    if (!spi_init_done)
        return -1;
    
    offset = start_pg * flash_info.nand_parm.page_size;
    len = flash_info.nand_parm.page_size + flash_info.nand_parm.spare_size;
    
    //ret = flash_info.write(&flash_info, COMMAND_QUAD_WRITE_PAGE, offset, len, tx_buf);
    ret = flash_info.write(&flash_info, COMMAND_WRITE_PAGE, offset, len, tx_buf);
    if (ret)
        return -1;
    
    return 0;
}

/* Flash sector erase, 
 * sector: sector id
 * sector_cnt: how many sectors
 * 0 for success, otherwise for fail 
 */
int FTSPI020_Erase_Sector(unsigned int sector, unsigned int sector_cnt)
{
    int ret, offset, len;
    
    if (!spi_init_done)
        return -1;
    
    offset = sector * flash_info.erase_sector_size;
    len = sector_cnt * flash_info.erase_sector_size;
    
    ret = flash_info.erase(&flash_info, SECTOR_ERASE, offset, len);
    if (ret)    
        return -1;
    
    return 0;
}

/* Flash chip erase
 * 0 for success, otherwise for fail 
 */
int FTSPI020_Erase_Chip(void)
{
	int	ret;
	
    if (!spi_init_done)
        return -1;
    
    ret = flash_info.erase_all(&flash_info);
    if (ret)    
        return -1;
    
    return 0;
}

/* Update the flash config
 */
int FTSPI020_Update_Param(unsigned int divisor, unsigned int pagesz_log2, unsigned int secsz_log2, unsigned int chipsz_log2)
{
    if (!spi_init_done)
        return -1;
    
    flash_info.page_size = (0x1 << pagesz_log2);
    flash_info.size = (0x1 << chipsz_log2);
    flash_info.erase_sector_size = (0x1 << secsz_log2);
    flash_info.nr_pages = (0x1 << (chipsz_log2 - pagesz_log2));
#if 0   //only need to load u-boot, don't care 4 bytes mode
    if(chipsz_log2 > 24)//over 16MB
        flash_info.mode_3_4 = 4;
    else
        flash_info.mode_3_4 = 3;
#endif            
    //if (divisor)
    //    ftspi020_divider(divisor);/* romCode has set it */
	
	//if (flash_info.mode_3_4 == 4)
	//    spi_dataflash_en4b(&flash_info, 1);
     
	return 0;
}

/* Update the flash config for SPI NAND
 */
int FTSPI020_Nand_Update_Param(INT32U divisor, INT32U u16_chip_block, INT32U u16_block_pg, INT32U u16_page_size, INT32U u16_spare_size_in_page)
{
    if (!spi_init_done)
        return -1;

    //KPrint("MSG: divisor=%d, chip_block=%d, block_pg=%d, page_size=%d, spare_size=%d\n", divisor, 
    //				u16_chip_block, u16_block_pg, u16_page_size, u16_spare_size_in_page);

    flash_info.nand_parm.chip_block = u16_chip_block;
    flash_info.nand_parm.block_pg = u16_block_pg;
    flash_info.nand_parm.page_size = u16_page_size;
    flash_info.nand_parm.spare_size = u16_spare_size_in_page;
    flash_info.nand_parm.block_size = u16_block_pg * u16_page_size;

    if (divisor)
        ftspi020_divider(divisor);

    //KPrint("MSG: spi nand info: page_size=%d, block_size=%d, spare_size=%d\n",
    //        flash_info.nand_parm.page_size, flash_info.nand_parm.block_size, flash_info.nand_parm.spare_size);

	return 0;
}

unsigned int fLib_NOR_Copy_Image(unsigned int addr)
{
    INT8U       i;
    unsigned int      start_pg, image_size;
    NOR_SYSHDR  *sys_hdr;

	memset((char *)addr, 0, 8); //clear the memory for signature part
	if (FTSPI020_PgRd(0, (INT8U *)addr, sizeof(NOR_SYSHDR)) < 0)
	{
	    KPrint("MSG: spi_pgrd fail");
	    goto fail;
	}

    if (sal_memcmp ((char *)addr, SS_SIGNATURE, 0x6) != 0)
    {
    		KPrint("SS_SIGNATURE not correct:"); 
        for (i = 0; i < 6; i ++)
            KPrint("<%x>", ((INT8U*)addr)[i]);
        KPrint("\n");    
        //goto fail;
	}
    
    if ((((INT8U*)addr)[0xFE] != 0x55) || (((INT8U*)addr)[0xFF] != 0xAA))
    {   
    		KPrint("0x55AA not correct\n");         
        //goto fail;
	}
	
	sys_hdr = (NOR_SYSHDR *)addr;

	/* update the parameter */
	FTSPI020_Update_Param(sys_hdr->norfixup.clk_div,
	              sys_hdr->norfixup.pagesz_log2, 
	              sys_hdr->norfixup.secsz_log2, 
	              sys_hdr->norfixup.chipsz_log2);
	//KPrint("load addr = 0x%x, size = 0x%x\n", sys_hdr->image[0].addr, sys_hdr->image[0].size);  			
	//KPrint("secsz=0x%x, pagesz=0x%x\n", 1 << sys_hdr->norfixup.secsz_log2, 1 << sys_hdr->norfixup.pagesz_log2);  			
	/* 
	 * Note: From now on, the R/W data section can't be used anymore. Because the boot code overwrites R/W section including
	 *       ZI section....
	 */
	/* Load front 20k into SRAM */
	start_pg = sys_hdr->image[0].addr / flash_info.page_size;//(1 << (sys_hdr->norfixup.secsz_log2 - sys_hdr->norfixup.pagesz_log2));
		
	image_size = sys_hdr->image[0].size;
	    	    
	if (FTSPI020_PgRd(start_pg, (INT8U *)addr, image_size) < 0) 
	{
	    KPrint("MSG: Load body fail");
	    goto fail;
	}
	/* exit 4byte mode */
	//if (flash_info.mode_3_4 == 4)
	//    spi_dataflash_en4b(&flash_info, 0);	
	KPrint("Boot image offset: 0x%x. size: 0x%x. Booting Image .....\n", start_pg * flash_info.page_size, image_size);
	    
	return (start_pg * flash_info.page_size);
	
fail:
	return -1;
}
/* -1 for search fail, others for success to find good block number */
int spi_nand_get_next_good_block(UINT32 cur_blk)
{
    int start_pg, page_len, goodblk, bFound = -1;
    INT8U page[FLASH_NAND_PAGE_SZ + FLASH_NAND_SPARE_SZ];

    for (goodblk = cur_blk; goodblk < flash_info.nand_parm.chip_block; goodblk ++)
    {    	
        start_pg = goodblk * flash_info.nand_parm.block_pg;
        page_len = flash_info.nand_parm.page_size + flash_info.nand_parm.spare_size;

        if (FTSPI020_PgRd(start_pg, (INT8U *) page, page_len) < 0)
        {
            KPrint("MSG: spi_pgrd fail");
            break;
        }

        if (page[FLASH_NAND_PAGE_SZ] == 0xFF)
        {
            bFound = goodblk;
            //KPrint("Block %d is good\n", cur_blk);
            break;
        }
    }

    return bFound;
}

unsigned int fLib_SPI_NAND_Copy_Image(unsigned int addr)
{
    INT8U i;//, j;
    INT8U *tmp_addr;
    unsigned int      start_pg, image_size, tmp_size, read_size, read_block_num, which_block;
    SPI_NAND_SYSHDR  *sys_hdr;

	//KPrint("read SPI_NAND_SYSHDR\n");

	memset((char *)addr, 0, 8); //clear the memory for signature part
	if (FTSPI020_PgRd(0, (INT8U *)addr, sizeof(SPI_NAND_SYSHDR)) < 0)
	{
	    KPrint("MSG: spi_pgrd fail");
	    goto fail;
	}
#if 0
    for (i = 0; i < 128; i ++){//print data
    	for (j = 0; j < 16; j ++)
        KPrint("<0x%x>", ((INT8U*)addr)[(i * 16) + j]);
        
      KPrint("\n");  
    }
#endif
    if (sal_memcmp ((char *)addr, SS_SIGNATURE, 0x6) != 0)
    {
    		KPrint("SS_SIGNATURE not correct\n");
        for (i = 0; i < 6; i ++)
            KPrint("<0x%x>", ((INT8U*)addr)[i]);
            
        goto fail;
	}
    if ((((INT8U*)addr)[0x1FE] != 0x55) || (((INT8U*)addr)[0x1FF] != 0xAA))
    {           
        KPrint("check 55AA fail\n");
        //goto fail;
	}

	sys_hdr = (SPI_NAND_SYSHDR *)addr;

	/* update the parameter */
	FTSPI020_Nand_Update_Param(sys_hdr->nandfixup.nand_clkdiv,
								sys_hdr->nandfixup.nand_numblks,
	              sys_hdr->nandfixup.nand_numpgs_blk,
	              sys_hdr->nandfixup.nand_pagesz,
	              sys_hdr->nandfixup.nand_sparesz_inpage);
	//KPrint("load addr = 0x%x, size = 0x%x\n", sys_hdr->image[0].addr, sys_hdr->image[0].size);  			
	//KPrint("secsz=0x%x, pagesz=0x%x\n", 1 << sys_hdr->norfixup.secsz_log2, 1 << sys_hdr->norfixup.pagesz_log2);  			
	/* 
	 * Note: From now on, the R/W data section can't be used anymore. Because the boot code overwrites R/W section including
	 *       ZI section....
	 */
	//KPrint("read u-boot\n");

	/* Load front 20k into SRAM */	
	start_pg = sys_hdr->image[0].addr / flash_info.nand_parm.page_size;		
	tmp_size = image_size = sys_hdr->image[0].size;
	
	read_block_num = (image_size + flash_info.nand_parm.block_size - 1) / flash_info.nand_parm.block_size;
	which_block = start_pg / flash_info.nand_parm.block_pg;
	tmp_addr = (INT8U *)addr;

	KPrint("Boot image offset: 0x%x. size: 0x%x. Load Image and Booting .....\n", sys_hdr->image[0].addr, sys_hdr->image[0].size);

	for(i = 0; i < read_block_num; i ++){
		which_block = spi_nand_get_next_good_block(which_block);
		
		if(tmp_size >= flash_info.nand_parm.block_size){
			tmp_size -= flash_info.nand_parm.block_size;
			read_size = flash_info.nand_parm.block_size;
		}
		else
			read_size = tmp_size;

		if (FTSPI020_PgRd(which_block * flash_info.nand_parm.block_pg, tmp_addr, read_size) < 0) 
		{
		    KPrint("MSG: Load body fail");
		    goto fail;
		}	
		tmp_addr += read_size;
		which_block++;	
	}

	/* exit 4byte mode */
	//if (flash_info.mode_3_4 == 4)
	//    spi_dataflash_en4b(&flash_info, 0);	
	    
	return sys_hdr->image[0].addr;
	
	
fail:
	return -1;
}