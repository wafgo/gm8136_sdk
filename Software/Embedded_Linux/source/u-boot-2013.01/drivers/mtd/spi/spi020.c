#include <common.h>
#include <asm/io.h>
#include <config.h>
#include "spi020_flash.h"
#include <config.h>

void FTSPI020_W32BIT(int offset, unsigned int data)
{
    //printf("w reg=0x%x, data=0x%x\n",offset, data);
    *((volatile u32 *)(CONFIG_SPI020_BASE + offset)) = data;
}

int FTSPI020_R32BIT(int offset)
{
    unsigned int data;
    data = *((volatile u32 *)(CONFIG_SPI020_BASE + offset));
    //printf("r reg=0x%x, data=0x%x\n",offset, data);
    return data;
}
/* ---------------------------------------------------------------------------------------
 * Internal functions
 * ---------------------------------------------------------------------------------------
 */
int ftspi020_Start_DMA(u32 Channel,	// use which channel for AHB DMA, 0..7
		      u32 SrcAddr,	// source begin address
		      u32 DstAddr,	// dest begin address
		      u32 Size,	    // total bytes
		      u32 SrcWidth,	// source width 8/16/32 bits -> 0/1/2
		      u32 DstWidth,	// dest width 8/16/32 bits -> 0/1/2
		      u32 SrcSize,	// source burst size, How many "SrcWidth" will be transmmited at one times ?
		      u32 SrcCtrl,	// source address change : Inc/dec/fixed --> 0/1/2
		      u32 DstCtrl,	// dest address change : Inc/dec/fixed --> 0/1/2
		      u32 Priority,	// priority for this chaanel 0(low)/1/2/3(high)
		      u32 Mode)	    // Normal/Hardwire,   0/1
{
    u32 direction;
    int dma_ch;
    int remain;

    //printf("%s in\n", __FUNCTION__);
    direction = (SrcCtrl == 0) ? DMA_MEM2SPI020 : DMA_SPI0202MEM;

    do {
        remain = Size > 3 * 1024 * 1024 ? 3 * 1024 * 1024 : Size;

        dma_ch = Dma_go_trigger((u32 *) SrcAddr, (u32 *) DstAddr, remain, NULL, direction, 1);
        if (dma_ch >= 0) {
            Dma_wait_timeout(dma_ch, 0);
        } else {
            printf("%s exit 0\n", __FUNCTION__);
            return 0;
        }

        Size -= remain;

        if (SrcCtrl == 0)       /* increase */
            SrcAddr += remain;
        if (DstCtrl == 0)
            DstAddr += remain;
    } while (Size > 0);

    //printf("%s exit 1\n", __FUNCTION__);
    return 1;
}

int ftspi020_scu_init(void)
{
    unsigned int reg = 0;

#ifndef CONFIG_CMD_FPGA
    /* Check for SPI function */
	reg = readl(CONFIG_PMU_BASE + 0x4);
#ifdef CONFIG_PLATFORM_GM8210
	if ((reg & 0x00C00000) != 0x00C00000) {   			/* BIT22 0: NOR, 1: SPI/NAND, BIT23 0: NAND, 1: SPI */
#else	
    if ((reg & (1 << 23)) != (1 << 23)) {   			/* BIT23 0: NAND, 1: SPI */
#endif    
    		printf("Not for SPI jump setting\n");								
      	return -1;
   	}		
#endif		
#ifdef CONFIG_SPI_DMA	
	/* Enable DMA020 clock */
	reg = readl(CONFIG_PMU_BASE + 0xB4);
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
	writel(reg & (~(1 << 8)), CONFIG_PMU_BASE + 0xB4);
#else
	writel(reg & (~(1 << 16)), CONFIG_PMU_BASE + 0xB4);
#endif		
		
    /* DMA init */
    Dma_init((1 << REQ_SPI020RX) | (1 << REQ_SPI020TX));		
#endif
#ifdef CONFIG_PLATFORM_GM8210
	reg = readl(CONFIG_PMU_BASE + 0x5C);
	writel(reg | (1 << 24), CONFIG_PMU_BASE + 0x5C);
#endif
		  
    return 0;
}

/* SPI Read Status Register. It responds the status from FLASH.
 */
void ftspi020_read_status(u8 * status)
{	
	*status = FTSPI020_8BIT(SPI_READ_STATUS);
	//printf("ftspi020_read_status=%x\n", *status);
}

/* Reset SPI020 controller.
 */
void ftspi020_reset_hw(void)
{
	FTSPI020_W32BIT(CONTROL_REG, FTSPI020_R32BIT(CONTROL_REG) | BIT8);
	while(FTSPI020_R32BIT(CONTROL_REG) | BIT8);
}

/* Set spi_clk_mode while IDLE state.
 * 0: mode0, sck_out will be low at the IDLE state
 * 1: mode3, sck_out will be high at the IDLE state
 */
void ftspi020_operate_mode(u8 mode)
{
	FTSPI020_W32BIT(CONTROL_REG, FTSPI020_R32BIT(CONTROL_REG) & (~BIT4));
	FTSPI020_W32BIT(CONTROL_REG, FTSPI020_R32BIT(CONTROL_REG) | ((mode << 4) & BIT4));
}

void ftspi020_busy_location(u8 location)
{
	FTSPI020_W32BIT(CONTROL_REG, FTSPI020_R32BIT(CONTROL_REG) & (~(BIT18 | BIT17 | BIT16)));
	FTSPI020_W32BIT(CONTROL_REG, FTSPI020_R32BIT(CONTROL_REG) | ((location << 16) & (BIT18 | BIT17 | BIT16)));
}

void ftspi020_divider(u8 divider)
{
	u8 val;
        
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
		printf("Not valid divider value %d\n", divider);
		return ;
	}

	FTSPI020_W32BIT(CONTROL_REG, FTSPI020_R32BIT(CONTROL_REG) & (~(BIT1 | BIT0)));
	FTSPI020_W32BIT(CONTROL_REG, FTSPI020_R32BIT(CONTROL_REG) | ((val) & (BIT1 | BIT0)));
}

/* polling command complete interrupt until the interrupt comes.
 */
void ftspi020_wait_cmd_complete(void)
{
    volatile u32 value, loop = 0x10000;

    do {
    		//udelay (1);
        value = FTSPI020_R32BIT(INTR_STATUS_REG);
        if (value & cmd_complete)
            break;
        loop --;
    } while (loop > 0);

    if(loop <= 0)
       printf("wait cmd complete timeout\n");
        
    FTSPI020_W32BIT(INTR_STATUS_REG, FTSPI020_R32BIT(INTR_STATUS_REG) | 0x1);  //write 1 clear 
}

void ftspi020_wait_rxfifo_ready(void)
{
    volatile u32 value;

    do {
    		//udelay (1);
        value = FTSPI020_R32BIT(STATUS_REG);
        if (value & (1 << rx_ready))
            break;
    } while (1);     
}

void ftspi020_wait_txfifo_ready(void)
{
    volatile u32 value;

    do {
    		//udelay (1);
        value = FTSPI020_R32BIT(STATUS_REG);
        if (value & (1 << tx_ready))
            break;
    } while (1);     
}

u8 ftspi020_issue_cmd(struct ftspi020_cmd *command)
{
	u32 cmd_feature1, cmd_feature2;
	
	FTSPI020_W32BIT(SPI_FLASH_ADDR, command->spi_addr);

	cmd_feature1 = ((command->conti_read_mode_en & 0x1) << 28) |
	    ((command->ins_len & 0x3) << 24) | ((command->dum_2nd_cyc & 0xFF) << 16) | ((command->addr_len & 0x7) << 0);
	FTSPI020_W32BIT(SPI_CMD_FEATURE1, cmd_feature1);

	FTSPI020_W32BIT(SPI_DATA_CNT, command->data_cnt);

	cmd_feature2 = ((command->ins_code & 0xFF) << 24) |
	    ((command->conti_read_mode_code & 0xFF) << 16) |
	    ((command->start_ce & 0x3) << 8) |
	    ((command->spi_mode & 0x7) << 5) |
	    ((command->dtr_mode & 0x1) << 4) |
	    ((command->read_status & 0x1) << 3) |
	    ((command->read_status_en & 0x1) << 2) | ((command->write_en & 0x1) << 1) | ((command->intr_en & 0x1) << 0);
    //printf("cmd 0x%x = 0x#%x, cmd 0x%x = 0x#%x\n",INTR_CONTROL_REG,FTSPI020_R32BIT(INTR_CONTROL_REG),INTR_STATUS_REG,FTSPI020_R32BIT(INTR_STATUS_REG));
    //printf("spi cmd = 0x#%x, 0x#%x, 0x#%x, 0x#%x\n",command->spi_addr,cmd_feature1,command->data_cnt,cmd_feature2);
	FTSPI020_W32BIT(SPI_CMD_FEATURE2, cmd_feature2);
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
	
	/* update to the controller */
	ftspi020_issue_cmd(spi_cmd);
}

/* direction: 1 is read, 0 is write */
void ftspi020_data_access(u8 direction, u8 *addr, u32 len)
{
    if (direction) {
        /* read direction */

#ifdef CONFIG_SPI_DMA        
	    ftspi020_Start_DMA(SPI020_CHANNEL, 
	                (u32) (CONFIG_SPI020_BASE + SPI020_DATA_PORT),    /* source */
					(u32) addr,   /* destination */ 
					len,            /* total bytes */ 
					0x00,           /* source width 8/16/32 bits -> 0/1/2 */
					0x00,           /* dest width 8/16/32 bits -> 0/1/2 */
					4,              /* source burst size, How many "SrcWidth" will be transmmited at one times ? */
					2,              /* source address change : Inc/dec/fixed --> 0/1/2 */
					0,              /* dest address change : Inc/dec/fixed --> 0/1/2 */
					0,              /* priority for this chaanel 0(low)/1/2/3(high) */
					HW_HANDSHAKE_MODE); // Normal/Hardwire,   0/1
#else
	    int i, j, loop;
        u32 *tmp, align = (u32)addr;

		tmp = (u32 *)addr;
		if(len > (rxfifo_depth * 4)) {
		    loop = len / (rxfifo_depth * 4);

		    if(align & 0x3) {
        		for(i = 0; i < loop; i ++){
        			ftspi020_wait_rxfifo_ready();
        			for(j = 0; j < (rxfifo_depth * 4); j ++)
        				*addr++ = FTSPI020_8BIT(SPI020_DATA_PORT);
        		}
		    } else {
        		for(i = 0; i < loop; i ++){
        			ftspi020_wait_rxfifo_ready();
        			for(j = 0; j < rxfifo_depth; j ++)
        				*tmp++ = FTSPI020_R32BIT(SPI020_DATA_PORT);
        		}
        		addr += (loop * rxfifo_depth * 4);    				        
		    }
    		len = len - (loop * rxfifo_depth * 4);
		}
		if(len > 0) {
		    ftspi020_wait_rxfifo_ready();
			for(j = 0; j < len; j ++)
				*addr++ = FTSPI020_8BIT(SPI020_DATA_PORT);			    
		}
#endif					    
    }   
    else {
        /* write direction */

#ifdef CONFIG_SPI_DMA        
        ftspi020_Start_DMA(SPI020_CHANNEL, 
                        (u32) addr,          /* source */
					    (u32) (CONFIG_SPI020_BASE + SPI020_DATA_PORT),/* destination */ 
					    len,    /* total bytes */
					    0x00,   /* source width 8/16/32 bits -> 0/1/2 */
					    0x00,   /* dest width 8/16/32 bits -> 0/1/2 */
					    4,      /* source burst size, How many "SrcWidth" will be transmmited at one times ? */
					    0,      /* source address change : Inc/dec/fixed --> 0/1/2 */
					    2,      /* dest address change : Inc/dec/fixed --> 0/1/2 */
					    0,      /* priority for this chaanel 0(low)/1/2/3(high) */
					    HW_HANDSHAKE_MODE);     // Normal/Hardwire,   0/1
#else
		int i, j, loop;
        u32 *tmp, align = (u32)addr;

		tmp = (u32 *)addr;
		if(len > (txfifo_depth * 4)) {
		    loop = len / (txfifo_depth * 4);

		    if(align & 0x3) {
        		for(i = 0; i < loop; i ++){
        			ftspi020_wait_txfifo_ready();
        			for(j = 0; j < (txfifo_depth * 4); j ++)
        				FTSPI020_8BIT(SPI020_DATA_PORT) = *addr++;
        		}
		    } else {
        		for(i = 0; i < loop; i ++){
        			ftspi020_wait_txfifo_ready();
        			for(j = 0; j < txfifo_depth; j ++)
        				FTSPI020_W32BIT(SPI020_DATA_PORT, *tmp++);
        		}
        		addr += (loop * rxfifo_depth * 4); 
    	    }
    		len = len - (loop * txfifo_depth * 4);    		   					
		}
		if(len > 0) {
		    ftspi020_wait_txfifo_ready();
			for(j = 0; j < len; j ++)
				FTSPI020_8BIT(SPI020_DATA_PORT) = *addr++;
		}
#endif					
    }
}

/* Entry point. Retrun 0 for success. < 0 for fail
 */
void spi_init(void)
{
#ifdef CONFIG_CMD_FPGA
	printf("SPI020 Revision:0x%x\n", FTSPI020_32BIT(REVISION));
	
	/* init the pin mux */
	ftspi020_scu_init();
	
	if (1)  /* init spi020 */
    {
    	/* reset spi020 */
    	ftspi020_reset_hw();
    	/* rx/tx threshold is 8/8, dma is enable */
    	
    	/* set mode0 or mode3 for the CLK */
    	ftspi020_operate_mode(mode3);
    	ftspi020_divider(8);    /* it is slow in romCode stage */
    	ftspi020_busy_location(BUSY_BIT0);
    }
#endif
    
#ifdef CONFIG_SPI_DMA
    FTSPI020_8BIT(INTR_CONTROL_REG) = 0x1;
#else
    FTSPI020_8BIT(INTR_CONTROL_REG) = 0x0;
#endif	
}
