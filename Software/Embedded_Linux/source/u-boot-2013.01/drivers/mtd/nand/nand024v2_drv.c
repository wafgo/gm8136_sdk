#include <common.h>
#include <asm/io.h>

#if !defined(CFG_NAND_LEGACY)

//#undef CONFIG_GM_AHBDMA//justin
//======================================================

struct collect_data_buf p_buf;
extern struct flash_info flash_parm;
extern unsigned char pgbuf[];
UINT32 cq4_default;
u8 cmd_status;
u8 spare_user_size = 16;

BOOL Row_addr_cycle(UINT8 u8_cycle);
BOOL Column_addr_cycle(UINT8 u8_cycle);

/* offset 0x24, ecc_err_fail_chx for DATA part
 */
BOOL ECC_correction_failed(UINT8 u8_channel)
{
    if (FTNANDC023_8BIT(ECC_INTR_STATUS) & (0x1 << u8_channel)) {
        FTNANDC023_8BIT(ECC_INTR_STATUS) |= (0x1 << u8_channel);
        return 1;
    } else {
        return 0;
    }
}

/* 0ffset 0x8, 1 for masking block function */
BOOL ECC_mask_status(UINT8 u8_channel)
{
    if ((FTNANDC023_8BIT(ECC_Control) & (1 << u8_channel)) == 0)
        return 0;

    return 1;
}

/* low enable write protect, high disable write protect */
void write_protect(int mode)
{
#ifdef CONFIG_GPIO_WP
	*(volatile unsigned int *)(CONFIG_GPIO2_BASE + 0x08) |= (1 << 30);
	if(mode)
		*(volatile unsigned int *)(CONFIG_GPIO2_BASE + 0x00) &= ~(1 << 30);
	else
		*(volatile unsigned int *)(CONFIG_GPIO2_BASE + 0x00) |= (1 << 30);
#endif
}		

int Check_command_status(UINT8 u8_channel, UINT8 detect)//=ftnandc024_nand_wait
{
    volatile u8 cmd_comp_sts, sts_fail_sts;
    volatile u32 intr_sts, ecc_intr_sts;
    volatile u8 ecc_sts_for_data, ecc_sts_for_spare;
    int ret = 1, loop_time = 0x50000;

    NANDC_DEBUG1("%s in\n", __FUNCTION__);
	while (loop_time) {
		intr_sts = FTNANDC023_32BIT(NANDC_Interrupt_Status);
		//printf("<0x%x>", intr_sts);
		cmd_comp_sts = ((intr_sts & 0xFF0000) >> 16);

		if (cmd_comp_sts & (1 << (u8_channel))) {
			// Clear the intr status when the cmd complete occurs.
			FTNANDC023_32BIT(NANDC_Interrupt_Status) = intr_sts;
			NANDC_DEBUG1("clear cmd_comp\n");
			
			cmd_status = CMD_SUCCESS;
			sts_fail_sts = (intr_sts & 0xFF);
				
			if (sts_fail_sts & (1 << (u8_channel))) {//justin
				printf("STATUS FAIL\n");
				cmd_status |= CMD_STATUS_FAIL;
				ret = 0;
				//ftnandc024_soft_reset(mtd);
			}
			
			ecc_intr_sts = FTNANDC023_32BIT(ECC_INTR_STATUS);
			// Clear the ECC intr status
			FTNANDC023_32BIT(ECC_INTR_STATUS) = ecc_intr_sts;
			//if(ecc_intr_sts)
			//    printk("<ecc sts=0x%x, flow=0x%x>",ecc_intr_sts,state);	
		
			// ECC failed on data		
			ecc_sts_for_data = (ecc_intr_sts & 0xFF); 
			if (ecc_sts_for_data & (1 << u8_channel)) {
				cmd_status |= CMD_ECC_FAIL_ON_DATA;
				printf("CMD_ECC_FAIL_ON_DATA\n");
				ret = 0;
			}
			ecc_sts_for_spare = ((ecc_intr_sts & 0xFF0000) >> 16); 
			// ECC failed on spare	
			if (ecc_sts_for_spare & (1 << u8_channel)) {
				cmd_status |= CMD_ECC_FAIL_ON_SPARE;
				printf("CMD_ECC_FAIL_ON_SPARE\n");
				ret = 0;
			}
			break;
		}
		loop_time --;	
	}
	if(!loop_time)
	    printf("Wait Check_command_status timeout\n");
	
	NANDC_DEBUG1("%s out\n", __FUNCTION__);
	return ret;
}

void init_cmd_feature_default(struct command_queue_feature *cmd_feature)
{
#if 0
    memset(cmd_feature, 0, sizeof(struct command_queue_feature));

    cmd_feature->Flash_type = Legacy_flash;
    cmd_feature->Command_wait = Command_Wait_Disable;
    cmd_feature->BMC_region = BMC_Region0;
    cmd_feature->BMC_ignore = No_Ignore_BMC_Region_Status;
    cmd_feature->Byte_mode = Byte_Mode_Disable; //page mode
    cmd_feature->Spare_sram_region = Spare_sram_region0;
    cmd_feature->Command_handshake_mode = Command_Handshake_Enable;
    cmd_feature->Command_incremental_scale = Command_Inc_By_Page;
    cmd_feature->Complete_interrupt_enable = Command_Complete_Intrrupt_Enable;
#endif
    cq4_default = CMD_FLASH_TYPE(LEGACY_FLASH) | CMD_START_CE(0) | ((spare_user_size - 1) << 19);
}

/*
 * u8_flash_page_size: page size in kbytes
 * u16_flash_block_size: how many pages in a block
 */
BOOL Set_memory_attribute(UINT16 u16_spare_size_in_byte, UINT16 u16_flash_block_size,
                              UINT8 u8_flash_page_size)
{
    UINT32 u32_setting = FTNANDC023_32BIT(Memory_Attribute_Setting);

    if (u8_flash_page_size == 1) {
        u8_flash_page_size = 0;
    } else if (u8_flash_page_size == 2) {
        u8_flash_page_size = 1;
    } else if (u8_flash_page_size == 4) {
        u8_flash_page_size = 2;
    } else if (u8_flash_page_size == 8) {
        u8_flash_page_size = 3;
    }
    // Clear and set the field of spare size 
    u32_setting &= ~(0x3FF << 16);
    u32_setting |= ((u16_spare_size_in_byte & 0x3FF) << 16);
    // Clear and set the field of block size
    u32_setting &= ~(0x3FF << 2);
    u32_setting |= ((u16_flash_block_size & 0x3FF) << 2);
    // Clear and set the field of page size
    u32_setting &= ~(0x3);
    u32_setting |= (u8_flash_page_size & 0x3);

    FTNANDC023_32BIT(Memory_Attribute_Setting) = u32_setting;

    return 1;
}

/* Offset 0x200, check whether command queue is full. [15:8]cmdq_full
 */
UINT8 Command_queue_status_full(UINT8 u8_channel)
{
    int loop_time = 0x50000;
    
    NANDC_DEBUG1("%s\n", __FUNCTION__);
    FTNANDC023_32BIT(Global_Software_Reset) = 1;
    
    // Wait for the global reset is complete.
    while (loop_time){
        if (((FTNANDC023_32BIT(Command_Queue_Status) >> (u8_channel + 8)) & 0x1) == 0) {
            break;               // It's not full
        }
        loop_time --;
    }
    if(!loop_time)
        printf("Wait Command_queue_status_full timeout\n");
        
    return 0;
}

/* Register 0x150: NANDC Interrupt Enable Register
 */
BOOL NANDC023_interrupt_enable(UINT8 u8_channel, UINT8 u8_crc_failed, UINT8 u8_flash_status_failed)
{
    FTNANDC023_32BIT(NANDC_Interrupt_Enable) &= ~((1 << u8_channel) | (1 << (u8_channel + 8)));
    FTNANDC023_32BIT(NANDC_Interrupt_Enable) |=
        (UINT32) (((u8_flash_status_failed) << u8_channel) | (u8_crc_failed << (u8_channel + 8)));
    return 1;
}

BOOL Page_read_with_spare_setting(UINT8 u8_channel, UINT32 u32_row_addr, UINT8 u8_sector_offset,
                                  struct collect_data_buf * data_buf)
{
    struct command_queue_feature cmd_f;

	cmd_f.row_cycle = Row_address_2cycle;//justin
	cmd_f.col_cycle = Column_address_2cycle;
	cmd_f.cq1 = (u32_row_addr & 0xFFFFFF);
	cmd_f.cq2 = 0;
	cmd_f.cq3 = CMD_COUNT(data_buf->u32_data_length_in_bytes / flash_parm.u16_sector_size) | (u8_sector_offset & 0xFF);
	cmd_f.cq4 = CMD_COMPLETE_EN | cq4_default | CMD_INDEX(PAGE_READ_WITH_SPARE);

#ifdef FTNANDC023_USE_DMA
	cmd_f.cq4 |= CMD_DMA_HANDSHAKE_EN;
#endif

    Setting_feature_and_fire(u8_channel, &cmd_f);

    return 1;
}

BOOL Page_write_with_spare_setting(UINT8 u8_channel, UINT32 u32_row_addr, UINT8 u8_sector_offset,
                                   struct collect_data_buf * data_buf)
{
    struct command_queue_feature cmd_f;
    
    write_protect(0);

	cmd_f.row_cycle = Row_address_2cycle;
	cmd_f.col_cycle = Column_address_2cycle;
	cmd_f.cq1 = (u32_row_addr & 0xFFFFFF);
	cmd_f.cq2 = 0;
	cmd_f.cq3 = CMD_COUNT(data_buf->u32_data_length_in_bytes / flash_parm.u16_sector_size) | (u8_sector_offset & 0xFF);
	cmd_f.cq4 = CMD_COMPLETE_EN | cq4_default | CMD_INDEX(PAGE_WRITE_WITH_SPARE);
	
#ifdef FTNANDC023_USE_DMA
    cmd_f.cq4 |= CMD_DMA_HANDSHAKE_EN;
#endif
    Setting_feature_and_fire(u8_channel, &cmd_f);
    
    return 1;
}

/* u16_command_index = opcode
 */
BOOL Setting_feature_and_fire(unsigned char u8_channel, struct command_queue_feature *cmd_f)
{
    // Checking the command queue isn't full
    Command_queue_status_full(u8_channel);

    Row_addr_cycle(cmd_f->row_cycle);
    Column_addr_cycle(cmd_f->col_cycle);
    
    // Filled with the 1st. row addr
    FTNANDC023_32BIT(Command_Queue1(u8_channel)) = cmd_f->cq1;

    // Filled with the 2nd. row addr
    FTNANDC023_32BIT(Command_Queue2(u8_channel)) = cmd_f->cq2;

    // Filled with the 3rd. row addr
    FTNANDC023_32BIT(Command_Queue3(u8_channel)) = cmd_f->cq3;

    // Filled with the 4th. row addr
    FTNANDC023_32BIT(Command_Queue4(u8_channel)) = cmd_f->cq4;
    
    NANDC_DEBUG1("cq0 = 0x%x,", FTNANDC023_32BIT(Memory_Attribute_Setting));
    NANDC_DEBUG1("cq1 = 0x%x,", cmd_f->cq1);
    NANDC_DEBUG1("cq2 = 0x%x,", cmd_f->cq2);
    NANDC_DEBUG1("cq3 = 0x%x,", cmd_f->cq3);
    NANDC_DEBUG1("cq4 = 0x%x\n", cmd_f->cq4);
  
    return 1;
}

BOOL ftnand_Start_DMA(UINT32 Channel,   // use which channel for AHB DMA, 0..7
                      UINT32 SrcAddr,   // source begin address
                      UINT32 DstAddr,   // dest begin address
                      UINT32 Size,      // total bytes
                      UINT32 SrcWidth,  // source width 8/16/32 bits -> 0/1/2
                      UINT32 DstWidth,  // dest width 8/16/32 bits -> 0/1/2
                      UINT32 SrcSize,   // source burst size, How many "SrcWidth" will be transmmited at one times ?
                      UINT32 SrcCtrl,   // source address change : Inc/dec/fixed --> 0/1/2
                      UINT32 DstCtrl,   // dest address change : Inc/dec/fixed --> 0/1/2
                      UINT32 Priority,  // priority for this chaanel 0(low)/1/2/3(high)
                      UINT32 Mode)      // Normal/Hardwire,   0/1                          
{
    UINT32 direction;
    INT32S dma_ch;
    int remain;

    NANDC_DEBUG3("%s in\n", __FUNCTION__);
    direction = (SrcCtrl == 0) ? DMA_MEM2NAND : DMA_NAND2MEM;

    do {
        remain = Size > 3 * 1024 * 1024 ? 3 * 1024 * 1024 : Size;

        dma_ch = Dma_go_trigger((INT32U *) SrcAddr, (INT32U *) DstAddr, remain, NULL, direction, 1);
        if (dma_ch >= 0) {
            NANDC_DEBUG3("Dma_wait_timeout in, channel = %d\n", dma_ch);
            Dma_wait_timeout(dma_ch, 0);
            NANDC_DEBUG3("Dma_wait_timeout exit\n");
        } else {
            NANDC_DEBUG3("%s exit 0\n", __FUNCTION__);
            return 0;
        }

        Size -= remain;

        if (SrcCtrl == 0)       /* increase */
            SrcAddr += remain;
        if (DstCtrl == 0)
            DstAddr += remain;
    } while (Size > 0);

    NANDC_DEBUG3("%s exit 1\n", __FUNCTION__);
    return 1;
}

/* Read Data from spare sram or fifo, ok
 */
BOOL Data_read(UINT8 u8_channel, struct collect_data_buf * data_buf,
               struct flash_info * flash_readable_info, UINT8 u8_data_presence,
               UINT8 u8_spare_presence)
{
    UINT8 *u8_spare_buf, u8_spare_length_in_sector;
    UINT32 u32_i, u32_j, u32_k;
    UINT32 u32_spare_offset;
    UINT32 offset;
    UINT32 u32_spare_area_index;

    NANDC_DEBUG2("%s in=0x%x,%d,%d,%d\n", __FUNCTION__, data_buf->u8_data_buf,
                 data_buf->u32_data_length_in_bytes, u8_data_presence, u8_spare_presence);


    if (u8_data_presence == 1) {
        /* If only one data port, offset will 0 */
        offset = 0;
#ifdef CONFIG_GM_AHBDMA//justin
        ftnand_Start_DMA(0,     //don't care  
                         (UINT32) (FTNANDC023_DATA_PORT_BASE + offset), (UINT32) data_buf->u8_data_buf, data_buf->u32_data_length_in_bytes, 0x02, 0x02, 6, 2,   //SrcCtrl, source address change : Inc/dec/fixed --> 0/1/2
                         0,     //DstCtrl, dest address change : Inc/dec/fixed --> 0/1/2
                         3, Command_Handshake_Enable);
#else
        {
    		int i;
    		u32 *tmp;			
    		
    		tmp = (u32 *)data_buf->u8_data_buf;
    		for(i = 0; i < (data_buf->u32_data_length_in_bytes / 4); i ++){
    		    *tmp = FTNANDC023_DATA_PORT(offset);
    		    NANDC_DEBUG2("<0x%x>", *tmp);
    		    tmp++;
    		}
    		NANDC_DEBUG2("\n");
	    }
#endif
    }
    
    /* Read spare area */
    if (u8_spare_presence == 1) {
        if (data_buf->u8_bytemode == Byte_Mode_Enable) {
            u32_spare_offset = 0;

            for (u32_i = 0; u32_i < data_buf->u16_spare_length; u32_i++) {
                *((volatile UINT8 *)(data_buf->u8_spare_data_buf + u32_i)) =
                    FTNANDC023_8BIT(Spare_Sram_Access + u32_spare_offset + u32_i);
            }
        } else if (data_buf->u8_mode == Sector_mode) {
            unsigned char buffer[64];
            // Alloc the spare buf. We read the whole spare first. 
            // Filter the garbage data from this buffer and fill the data into the "real buf"(data_buf->u8_spare_buf).
            u8_spare_buf = (UINT8 *) buffer;
            memset(u8_spare_buf, 0, 64);
            u8_spare_length_in_sector = (64 / flash_readable_info->u16_sector_in_page); //how many bytes per sector

            for (u32_j = 0; u32_j < 1; u32_j++) {
                u32_spare_offset = 0;

                /* First step, read back whole spare sram into DDR first */
                for (u32_i = 0; u32_i < 64; u32_i += 4) {
                    *((volatile UINT32 *)(u8_spare_buf + u32_i)) =
                        FTNANDC023_32BIT(((Spare_Sram_Access + u32_spare_offset) + u32_i));
                    //printf("<a=%x>", FTNANDC023_32BIT((Spare_Sram_Access + u32_spare_offset) + u32_i));
                }

                // Second step, Filter the garbage data from buffer.
                u32_spare_area_index = 0;

                for (u32_i = 0; u32_i < flash_readable_info->u16_sector_in_page; u32_i++)       //go through all sectors in page
                {
                    for (u32_k = 0; u32_k < data_buf->u8_spare_size_for_sector; u32_k++) {
                        *((volatile UINT8 *)(data_buf->u8_spare_data_buf +
                                             (u32_j * data_buf->u16_spare_length) +
                                             u32_spare_area_index)) =
                            *((volatile UINT8 *)(u8_spare_buf + u32_i * u8_spare_length_in_sector +
                                                 u32_k));
                        u32_spare_area_index++;
                    }
                }
            }
        } else {
            NANDC_DEBUG2("%s exit 0\n", __FUNCTION__);
            return 0;
        }
    }
    NANDC_DEBUG2("%s exit 1\n", __FUNCTION__);
    return 1;
}

/* Data_write
 */
BOOL Data_write(UINT8 u8_channel, struct collect_data_buf * data_buf,
                struct flash_info * flash_readable_info, UINT8 u8_data_presence,
                UINT8 u8_spare_presence)
{
    //UINT8  u8_loop_times, 
    UINT8 *u8_spare_buf;
    UINT8 u8_total_spare_length_can_be_used_for_sector;
    UINT32 u32_i, u32_j, u32_k;
    UINT32 u32_spare_offset;
    UINT32 offset;
    UINT32 u32_spare_area_index;

    NANDC_DEBUG2("%s in=0x%x,%d\n", __FUNCTION__, data_buf->u8_data_buf,
                 data_buf->u32_data_length_in_bytes);

    if (u8_data_presence == 1) {
        offset = 0;
#ifdef CONFIG_GM_AHBDMA
        ftnand_Start_DMA(0,     //don't care                        
                         (UINT32) data_buf->u8_data_buf, FTNANDC023_DATA_PORT_BASE + offset, data_buf->u32_data_length_in_bytes, 0x02, 0x02, 6, 0,      //SrcCtrl, source address change : Inc/dec/fixed --> 0/1/2
                         2,     //DstCtrl, dest address change : Inc/dec/fixed --> 0/1/2
                         3, Command_Handshake_Enable);
#else
        {
    		int i;
    		u32 *tmp;			
    		
    		tmp = (u32 *)data_buf->u8_data_buf;
    		for(i = 0; i < (data_buf->u32_data_length_in_bytes / 4); i ++)
    		    FTNANDC023_DATA_PORT(offset) = *tmp++;
		}
#endif
    }

    if (u8_spare_presence == 1) {
        if (data_buf->u8_bytemode == Byte_Mode_Enable) {
            NANDC_DEBUG2("MSG: Data_write in Byte_Mode_Enable");
            u32_spare_offset = 0;
            for (u32_i = 0; u32_i < data_buf->u16_spare_length; u32_i++) {
                FTNANDC023_8BIT(Spare_Sram_Access + u32_spare_offset + u32_i) =
                    *((volatile UINT8 *)(data_buf->u8_spare_data_buf + u32_i));
            }
        } else if (data_buf->u8_mode == Sector_mode) {
            unsigned char buffer[64];

            NANDC_DEBUG2("MSG: Data_write in sector mode");

            u8_spare_buf = (UINT8 *) buffer;
            memset(u8_spare_buf, 0, 64);

            u8_total_spare_length_can_be_used_for_sector = (64 / flash_readable_info->u16_sector_in_page);      //how many bytes per sector

            for (u32_j = 0; u32_j < 1; u32_j++) {
                u32_spare_offset = 0;

                // Expand the data into 64 byte in order to fill spare sram.
                u32_spare_area_index = 0;
                for (u32_i = 0; u32_i < flash_readable_info->u16_sector_in_page; u32_i++) {
                    for (u32_k = 0; u32_k < data_buf->u8_spare_size_for_sector; u32_k++) {
                        *((volatile UINT8 *)(u8_spare_buf +
                                             u32_i * u8_total_spare_length_can_be_used_for_sector +
                                             u32_k)) =
                            *((volatile UINT8 *)(data_buf->u8_spare_data_buf +
                                                 (u32_j * data_buf->u16_spare_length) +
                                                 u32_spare_area_index));
                        u32_spare_area_index++;
                    }
                }

                for (u32_i = 0; u32_i < 64; u32_i = u32_i + 4) {
                    FTNANDC023_32BIT(Spare_Sram_Access + u32_spare_offset + u32_i) =
                        *((volatile UINT32 *)(u8_spare_buf + u32_i));
                }
            }
        } else {
            NANDC_DEBUG1("MSG: Data_write Error");
            return 0;
        }
    }

    NANDC_DEBUG2("%s exit 1\n", __FUNCTION__);
    return 1;
}

/* Offset 0x50C, reset nandc
 */
BOOL Global_software_reset(void)
{
    int loop_time = 0x50000;
    FTNANDC023_32BIT(Global_Software_Reset) = 1;
    // Wait for the global reset is complete.
    while (loop_time){
        if((FTNANDC023_32BIT(Global_Software_Reset) & 0x1) == 1)
            break;
        loop_time --;
    }
    if(!loop_time)
        printf("Wait Global_software_reset timeout\n");
 
    return 1;
}

BOOL AHB_slave_memory_space(UINT8 u8_ahb_slave_memory_size)
{
    FTNANDC023_8BIT(AHB_Slave_Memory_Space) &= ~0xff;
    FTNANDC023_8BIT(AHB_Slave_Memory_Space) |= u8_ahb_slave_memory_size;

    return 1;
}

BOOL Row_addr_cycle(UINT8 u8_cycle)
{
    // Clear the value setted before
    FTNANDC023_32BIT(Memory_Attribute_Setting) &= ~(0x3 << 13);
    // Set the value
    FTNANDC023_32BIT(Memory_Attribute_Setting) |= ((u8_cycle & 0x3) << 13);
    return 1;
}

BOOL Column_addr_cycle(UINT8 u8_cycle)
{
    // Clear the value setted before
    FTNANDC023_32BIT(Memory_Attribute_Setting) &= ~(0x1 << 12);
    // Set the value
    FTNANDC023_32BIT(Memory_Attribute_Setting) |= ((u8_cycle & 0x1) << 12);
    return 1;
}

/* Regiser 0x8, bit[15:8]: ecc_en
 */
BOOL ECC_function(UINT8 u8_channel, UINT8 u8_choice)
{
    // Clear the field
    FTNANDC023_8BIT(ECC_Control + 1) &= ~(1 << u8_channel);
    // Set the value
    FTNANDC023_8BIT(ECC_Control + 1) |= ((u8_choice & 0x1) << u8_channel);
    return 1;
}

/* offset 0x08 
*/
BOOL ECC_setting_base_size(UINT8 u8_ecc_base_size)
{
    // Clear the ecc base field
    FTNANDC023_32BIT(ECC_Control) &= ~(1 << 16);
    // Set the value
    FTNANDC023_32BIT(ECC_Control) |= (UINT32) ((u8_ecc_base_size & 0x1) << 16);
    return 1;
}

/* Regiser 0x8, bit[7:0]: ecc_err_mask
 */
BOOL ECC_mask(UINT8 u8_channel, UINT8 u8_mask)
{
    // Clear the field
    FTNANDC023_8BIT(ECC_Control) &= ~(1 << u8_channel);
    // Set the value
    FTNANDC023_8BIT(ECC_Control) |= ((u8_mask & 0x1) << u8_channel);
    return 1;
}

/* Offset 0x10, threshold number of ECC Error Bits Register. Data part.
 */
BOOL ECC_threshold_ecc_error(UINT8 u8_channel, UINT8 u8_threshold_num)
{
    // Clear the specified field.
    FTNANDC023_8BIT(ECC_threshold_ecc_error_num + u8_channel) &= ~0x7F;
    FTNANDC023_8BIT(ECC_threshold_ecc_error_num + u8_channel) = (u8_threshold_num - 1) & 0x7F;

    return 1;
}

/* Offset 0x18, Number of ECC Correction Capability bits register, Data part.
 */
BOOL ECC_correct_ecc_error(UINT8 u8_channel, UINT8 u8_correct_num)
{
    // Clear the specified field.
    FTNANDC023_8BIT(ECC_correct_ecc_error_num + u8_channel) &= ~0x7F;
    FTNANDC023_8BIT(ECC_correct_ecc_error_num + u8_channel) = (u8_correct_num - 1) & 0x7F;

    return 1;
}

/* ECC 0x18, data part, for one sector
 */
BOOL NANDC023_ecc_occupy_for_sector(void)
{
    UINT8 u8_ecc_correct_bits, u8_occupy_byte;

    u8_ecc_correct_bits = (FTNANDC023_8BIT(ECC_correct_ecc_error_num) & 0x7F) + 1;

    u8_occupy_byte = (u8_ecc_correct_bits * 14) / 8;
    if ((u8_ecc_correct_bits * 14) % 8 != 0) {
        u8_occupy_byte++;
    }
    return u8_occupy_byte;
}

/* offset 0x08 , no_ecc_parity
*/
BOOL ECC_leave_space(UINT8 u8_choice)
{
    // Clear the field
    FTNANDC023_32BIT(ECC_Control) &= ~(1 << 17);
    // Set the value
    FTNANDC023_32BIT(ECC_Control) |= ((u8_choice & 0x1) << 17);
    return 1;
}

/* Register 0x8, 0x10, 0x14 
 */
BOOL FTNANDC023_ecc_setting(UINT8 u8_channel, UINT8 u8_enable, INT8 s8_correct_bits,
                            INT8 s8_threshold_bits, UINT8 u8_leave_space_for_ecc,
                            struct flash_info * flash_readable_info)
{

    flash_readable_info->u32_spare_start_in_byte_pos =
        (flash_readable_info->u16_sector_size +
         NANDC023_ecc_occupy_for_sector()) * flash_readable_info->u16_sector_in_page;

    ECC_function(u8_channel, u8_enable);
    ECC_mask(u8_channel, 1);    //always mask ECC error, but we still can get interrupt
    ECC_threshold_ecc_error(u8_channel, s8_threshold_bits);
    ECC_correct_ecc_error(u8_channel, s8_correct_bits);

    /* 0x8 */
    if (u8_enable == 1) {
        ECC_leave_space(ecc_existence_between_sector);
    } else {
        ECC_leave_space(u8_leave_space_for_ecc);
    }

    return 1;
}

/*
 * u16_block_pg:  how many pages in a block
 * u16_page_size: page real size
 * u16_spare_size_in_page: how many spare size in a whole page
 * u8_status_bit: which bit of flash bus indicates status
 * u8_cmmand_bit: which bit of flash bus indicates command pass/fail
 * ecc_capability: 0 ~ 23 indicates 1 ~ 24 bits
 * ecc_base: 0/1 indiciates 512/1024 bytes
 */
void nand023_drv_load_config(INT32U u16_block_pg, INT32U u16_page_size,
                             INT32U u16_spare_size_in_page, INT32U u8_status_bit,
                             INT32U u8_cmmand_bit, INT32U ecc_capability, INT32U ecc_base,
                             INT32U row_cycle, INT32U col_cycle)
{
    /*
     * Init the nand controller was done in ROM Code stage. Here we just need to assign the database.
     */
    flash_parm.u16_page_size_kb = B_TO_KB(u16_page_size);
    flash_parm.u16_block_size_kb = u16_block_pg * flash_parm.u16_page_size_kb;  //unit is kbytes
    flash_parm.u16_spare_size_in_page = u16_spare_size_in_page; //bytes
    flash_parm.u8_status_bit = u8_status_bit;
    flash_parm.u8_cmmand_bit = u8_cmmand_bit;
    flash_parm.u8_ecc_capability = ecc_capability;
    flash_parm.ecc_base = ecc_base;
    flash_parm.u16_sector_size = (ecc_base == ECC_BASE_512) ? 512 : 1024;
    flash_parm.u16_page_in_block = u16_block_pg;
    flash_parm.u16_sector_in_page = u16_page_size / flash_parm.u16_sector_size;
    flash_parm.u8_row_cyc = (row_cycle > 4) ? 3 : row_cycle - 1;
    flash_parm.u8_col_cyc = (col_cycle > 2) ? 2 : col_cycle - 1;

    /* ECC is enabled in rom code */
    if (flash_parm.ecc_base == ECC_BASE_512) {
        /* Set DMA Burst Size */
        Dma_Set_BurstSz(DMA_MEM2NAND, DMA_BURST_128);
        Dma_Set_BurstSz(DMA_NAND2MEM, DMA_BURST_128);
    } else {
        /* Set DMA Burst Size */
        Dma_Set_BurstSz(DMA_MEM2NAND, DMA_BURST_256);
        Dma_Set_BurstSz(DMA_NAND2MEM, DMA_BURST_256);
    }
}

/*
 * Read nandc register
 */
UINT32 nand023_drv_regread(UINT32 offset)
{
    volatile unsigned int value;

    value = *(volatile unsigned int *)(FTNANDC023_BASE + offset);

    return value;
}

/* -------------------------------------------------------------------------------
 * Body
 * -------------------------------------------------------------------------------
 */
/* assign default value to read page table later */
void nand_load_default(NANDCHIP_T * chip)
{
    volatile unsigned int value;

    /* page size, spare ... */
    value = nand023_drv_regread(Memory_Attribute_Setting);

    switch ((value >> 16) & 0x3) {
    default:
    case 1:
        chip->pagesize = KB_TO_B(2);
        break;
    case 2:
        chip->pagesize = KB_TO_B(4);
        break;
    case 3:
        chip->pagesize = KB_TO_B(8);
        break;
    case 4:
        chip->pagesize = KB_TO_B(16);
        break;
    }

    printf("Parsing Memory_Attribute_Setting = 0x%x\n", value);
#if 1                           /* FIXME, the number of pages in a block starts from 0, instead of 1.  */
    if (((value >> 2) & 0xF) != 0xF) {
        volatile unsigned int tmp;
        tmp = ((value >> 2) & 0x3FF) - 1;

        value &= ~(0x3FF << 2);
        value |= (tmp << 2);
        FTNANDC023_32BIT(Memory_Attribute_Setting) = value;
        printf("new Memory_Attribute_Setting = 0x%x\n", value);
    }
#endif
    chip->blocksize = (((value >> 2) & 0x3FF) + 1) * chip->pagesize;
    chip->chipsize = KB_TO_B(16384);
    chip->sparesize = 64;//justin//(value >> 16) & 0x3FF;

    /* Sector size and ECC base */
    value = (nand023_drv_regread(ECC_Control) >> 16) & 0x1;
    chip->ecc_base = (value == 0x1) ? ECC_BASE_1024 : ECC_BASE_512;
    chip->sectorsize = (chip->ecc_base == ECC_BASE_1024) ? 1024 : 512;

    /* ECC capability */
    value = nand023_drv_regread(ECC_correct_ecc_error_num) & 0x7F;

    chip->ecc_capability = value + 1;

    /* status bit and command bit */
    value = nand023_drv_regread(NANDC_General_Setting);
    chip->status_bit = (value >> 12) & 0x7;
    chip->cmmand_bit = (value >> 8) & 0x7;

    /* row cycle and column cycle */
    value = nand023_drv_regread(Memory_Attribute_Setting);
    chip->row_cyc = ((value >> 13) & 0x3) + 1;
    chip->col_cyc = ((value >> 12) & 0x1) + 1;
}

/* Nandc023 init function
 * Many init function was done in ROM code. So we don't init the nandchip again.
 */
void nand023_drv_init(void)
{
		UINT8 u8_channel = 0;
#ifdef CONFIG_CMD_FPGA

    FTNANDC023_32BIT(Flash_AC_Timing0(0)) = 0x0f1f0f1f;//timing
    FTNANDC023_32BIT(Flash_AC_Timing1(1)) = 0x00007f7f;
    FTNANDC023_32BIT(Flash_AC_Timing2(2)) = 0x7f7f7f7f;
    FTNANDC023_32BIT(Flash_AC_Timing3(3)) = 0xff1f001f;
    
    FTNANDC023_32BIT(Memory_Attribute_Setting) = 0x130FC;//architecture setting 1, must match flash
    FTNANDC023_32BIT(Spare_Region_Access_Mode) = 0x3F8000;//architecture setting 2, must match flash
    FTNANDC023_32BIT(ECC_correct_ecc_error_num) = 0x5;//ECC bit
    
    ECC_function(u8_channel, 1);
        
    ECC_threshold_ecc_error(u8_channel, 5);
    ECC_correct_ecc_error(u8_channel, 5);

    /* perform CRC check for channel 0 */
    FTNANDC023_32BIT(NANDC_General_Setting) &= ~(0x3 << 24); //only one CE  
    AHB_slave_memory_space(0x40);//32KB
#endif

		ECC_mask(u8_channel, 1);    //always mask ECC error, but we still can get interrupt
    /* correction fail trigger the interrupt */
    FTNANDC023_32BIT(ECC_Interrupt_Enable) = hit_threshold_intr_enable | correct_fail_intr_enable;

    /* NANDC Interrupt Enable Register */
    FTNANDC023_32BIT(NANDC_Interrupt_Enable) = ECC_INTR_CORRECT_FAIL | ECC_INTR_THRES_HIT;

    /* clear all interrupt status in 0x154 */
    FTNANDC023_32BIT(NANDC_Interrupt_Status) = 0xffffffff;

    /* data invert */
    FTNANDC023_32BIT(NANDC_General_Setting) |= 0x2;
}

/* Clear the spare area of one page per CE in Spare SRAM 
 */
BOOL FTNANDC023_clear_specified_spare_sram(UINT8 u8_channel, struct collect_data_buf *data_buf)
{
    if (u8_channel || data_buf) {
    }

    memset((char *)(FTNANDC023_BASE + Spare_Sram_Access), 0, 64);

    return 1;
}

void nand023_read_page(UINT32 u32_row_addr)
{
    struct command_queue_feature cmd_feature;
    UINT32 *u32_sixth_word;

    NANDC_DEBUG1("%s in\n", __FUNCTION__);
    u32_sixth_word = (UINT32 *)&cmd_feature;
    
    NANDC_DEBUG1("%s in\n", __FUNCTION__);
    NANDC_DEBUG1("page num = %d, pgbuf = 0x%x\n", u32_row_addr, pgbuf);

    memset(&p_buf, 0, sizeof(struct collect_data_buf));
    p_buf.u8_data_buf = (UINT8 *) pgbuf;
    //translate to real length from sector
    p_buf.u16_data_length_in_sector = flash_parm.u16_sector_in_page;    //how many sectors    

    p_buf.u32_data_length_in_bytes = p_buf.u16_data_length_in_sector * flash_parm.u16_sector_size;
    /* prepare spare buf */
    p_buf.u8_spare_data_buf = (UINT8 *) pgbuf;
    p_buf.u8_spare_size_for_sector = USR_SPARE_SIZE;
    p_buf.u16_spare_length = p_buf.u8_spare_size_for_sector * p_buf.u16_data_length_in_sector;

//////////////////////////////////

    /* byte mode is disabled */
    p_buf.u8_bytemode = Byte_Mode_Disable;

    init_cmd_feature_default(&cmd_feature);

    //Clear spare area to ensure read spare data is not previous written value.
    FTNANDC023_clear_specified_spare_sram(NAND_CHANNEL, &p_buf);

    // Checking the command queue isn't full
    Command_queue_status_full(NAND_CHANNEL);

    Page_read_with_spare_setting(NAND_CHANNEL, u32_row_addr, 0, &p_buf);

    NANDC_DEBUG1("%s exit\n", __FUNCTION__);
}

BOOL NANDC023_check_spare_size(UINT8 u8_spare_mode, UINT8 u8_spare_size,
                               struct flash_info * flash_readable_info)
{
    UINT16 u16_occupy_spare_bytes;
    UINT16 u16_remaining_spare_space;

    // For data, how many spare bytes are needed 
    u16_occupy_spare_bytes =
        (flash_readable_info->u16_sector_in_page) * NANDC023_ecc_occupy_for_sector();
    // For spare pare, how many spare bytes are needed
    if (u8_spare_mode == Sector_mode) {
        // The spare data is protected by CRC in Sector mode and CRC needs 2 bytes for spare ECC 1 bit. 
        u16_occupy_spare_bytes =
            u16_occupy_spare_bytes /* data */  +
            2; /* spare */
    }
#if SPARE_PAGE_MODE_SUPPORT
    else if (u8_spare_mode == Page_mode) {
        // The spare data is also protected by ECC in Page mode.
        u16_occupy_spare_bytes = u16_occupy_spare_bytes + NANDC023_ecc_occupy_for_spare();
    }
#else
    else {
        return 0;
    }
#endif
    u16_remaining_spare_space =
        flash_readable_info->u16_spare_size_in_page - u16_occupy_spare_bytes;

    if (u8_spare_mode == Sector_mode) {
        if (u16_remaining_spare_space < spare_user_size) {
        		printf("User size %d < spare num %d\n", u16_remaining_spare_space, spare_user_size); 
            return 0;
        }
    }
#if SPARE_PAGE_MODE_SUPPORT
    else if (u8_spare_mode == Page_mode) {
        if (u16_remaining_spare_space < u8_spare_size) {
            return 0;
        }
    }
#else
    else {
        return 0;
    }
#endif
    return 1;
}

void Prepare_spare(INT8U * Buf_spare, int sec_cnt, int LSN_start)
{
    int i, offset = 0;

    for (i = 0; i < sec_cnt; i++) {
        Buf_spare[offset] = LSN_start & 0xFF;
        Buf_spare[offset + 1] = (LSN_start >> 8) & 0xFF;
        Buf_spare[offset + 2] = (LSN_start >> 16) & 0xFF;
        offset += USR_SPARE_SIZE;
    }
}

BOOL nand023_write_page(UINT32 u32_row_addr)
{
    struct command_queue_feature cmd_feature;
    INT8U spare[NANDC_MAX_SECTOR * USR_SPARE_SIZE];
    UINT32 *u32_sixth_word;

    u32_sixth_word = (UINT32 *)&cmd_feature;
    
    NANDC_DEBUG1("%s in\n", __FUNCTION__);
    NANDC_DEBUG1("page num = %d, pgbuf = 0x%x\n", u32_row_addr, pgbuf);

    memset(&p_buf, 0, sizeof(struct collect_data_buf));
    p_buf.u8_data_buf = (UINT8 *) pgbuf;
    //translate to real length from sector
    p_buf.u16_data_length_in_sector = flash_parm.u16_sector_in_page;

    p_buf.u32_data_length_in_bytes = p_buf.u16_data_length_in_sector * flash_parm.u16_sector_size;

    /* prepare spare */
    Prepare_spare(spare, flash_parm.u16_sector_in_page, LSN_MAGIC);
    p_buf.u8_spare_data_buf = spare;
    p_buf.u8_spare_size_for_sector = USR_SPARE_SIZE;
    p_buf.u16_spare_length = flash_parm.u16_sector_in_page * p_buf.u8_spare_size_for_sector;

    p_buf.u8_mode = Sector_mode;

//////////////////////////////////      
    /* byte mode is disabled */
    p_buf.u8_bytemode = Byte_Mode_Disable;

    init_cmd_feature_default(&cmd_feature);

    if (!NANDC023_check_spare_size(p_buf.u8_mode, p_buf.u8_spare_size_for_sector, &flash_parm)) {
        printf("MSG: User data size %d for spare is too large! \n", p_buf.u8_spare_size_for_sector);
        return 0;
    }

    /* Set commandq 0-4 words */
    Page_write_with_spare_setting(NAND_CHANNEL, u32_row_addr, 0, &p_buf);
    
#if 0
    Data_write(NAND_CHANNEL, &p_buf, &flash_parm, 1, 0);        //read data page
    printf("buf[0]=%x\n", pgbuf[0]);
    //Command FAILED      
    if (!Check_command_status(NAND_CHANNEL, 1)) {
        printf("MSG: check status fail in ftnandc023_read_buffer");
        return 0;
    }
#endif
    NANDC_DEBUG1("%s exit\n", __FUNCTION__);
    return 1;
}

/* if spare all 0xFF, CRC will fail */
BOOL spare_dirty(UINT32 len, UINT8 * buf)
{
    int i;
#if 0
    int j;
    for (i = 0; i < 8; i++)
        for (j = 0; j < 8; j++)
            printf("0x%x ", buf[i * 8 + j]);

    printf("\n");
#endif
    for (i = 0; i < USR_SPARE_SIZE; i++)
        if (buf[i] != 0xFF)
            return 1;
    return 0;
}

/* Read Page and spare
 */
BOOL nand023_drv_read_page_with_spare(UINT32 u32_row_addr,
                                      UINT8 u8_sector_offset,
                                      UINT16 u16_sector_cnt,
                                      struct collect_data_buf * data_buf, int check_status)
{
    struct command_queue_feature cmd_feature;
    int ret;
    UINT32 *u32_sixth_word;

    NANDC_DEBUG1("nand023_drv_read_page_with_spare, startpg = 0x%x, cnt = 0x%x\n", u32_row_addr, u16_sector_cnt);
    
    u32_sixth_word = (UINT32 *)&cmd_feature;
    /* Spare Length check
     */
    if (data_buf->u8_mode == Sector_mode) {
        if (data_buf->u16_spare_length != (u16_sector_cnt * data_buf->u8_spare_size_for_sector)) {
            printf("MSG: f1");
            return 0;
        }
    } else if (data_buf->u8_mode == Page_mode) {
        /* do nothing, fall through */
    } else
        return 0;

    if (data_buf->u16_spare_length == 0) {
        printf("MSG: f2");
        return 0;
    }
    if (data_buf->u8_spare_data_buf == NULL) {
        printf("MSG: f3");
        return 0;
    }

    /* Data Length check
     */
    if (data_buf->u32_data_length_in_bytes != (u16_sector_cnt * flash_parm.u16_sector_size)) {
        printf("MSG: f4");
        return 0;
    }
    if (data_buf->u32_data_length_in_bytes == 0) {
        printf("MSG: f5");
        return 0;
    }
#if 0                           /* The address may be zero because DDR starts from 0. */
    if (data_buf->u8_data_buf == NULL) {
        NANDC_DEBUG1("MSG: f6");
        return 0;
    }
#endif
    /* byte mode is disabled */
    data_buf->u8_bytemode = Byte_Mode_Disable;
    data_buf->u16_data_length_in_sector = u16_sector_cnt;       //how many sectors

    init_cmd_feature_default(&cmd_feature);

    //Clear spare area to ensure read spare data is not previous written value.
    FTNANDC023_clear_specified_spare_sram(NAND_CHANNEL, data_buf);

    // Checking the command queue isn't full
    Command_queue_status_full(NAND_CHANNEL);

    Page_read_with_spare_setting(NAND_CHANNEL, u32_row_addr, u8_sector_offset, data_buf);
    //Setting_feature_and_fire(NAND_CHANNEL, Command(PAGE_READ_WITH_SPARE), Start_From_CE0, *u32_sixth_word);
    /* DMA starts */
    NANDC_DEBUG1("read data\n");
    Data_read(NAND_CHANNEL, data_buf, &flash_parm, 1, 0);       //read data page

    NANDC_DEBUG1("read spare\n");
    Data_read(NAND_CHANNEL, data_buf, &flash_parm, 0, 1);       //read spare

    //Command FAILED        
    ret =
        Check_command_status(NAND_CHANNEL,
                             spare_dirty(data_buf->u8_spare_size_for_sector,
                                         (UINT8 *) data_buf->u8_spare_data_buf));
    if (check_status && !ret) {
        printf("MSG: check status fail in ftnandc023_read_page_with_spare");
        return 0;
    }

    return 1;
}
#endif
