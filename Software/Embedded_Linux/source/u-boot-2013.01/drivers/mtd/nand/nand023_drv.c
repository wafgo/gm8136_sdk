#include <common.h>
#include <asm/io.h>

#if !defined(CFG_NAND_LEGACY)

#define FTNANDC023_USE_DMA
//======================================================

struct collect_data_buf p_buf;
extern struct flash_info flash_parm;
extern unsigned char pgbuf[];

BOOL Row_addr_cycle(UINT8 u8_cycle);
BOOL Column_addr_cycle(UINT8 u8_cycle);

extern unsigned char pgbuf[];
//======================================================

/*
	Function Name	: Manual_command_queue_setting
	
	Effect 			: Setting the command queue for specified channel. 
					  It's used when you set "Program Flow selection" to 1 in the command queue. 

	Parameter		:
		u8_channel		--> Channel ID 
		u32_manual_command_queue--> Accept the addr. of array which is used to setting the first five words in command queue.
	Return value	:	
		1	--> Function success
		0	--> Function fail 
*/
BOOL Manual_command_queue_setting(UINT8 u8_channel, UINT32 * u32_manual_command_queue)
{
    // Filled with the 1st. row addr
    FTNANDC023_32BIT(Command_Queue1(u8_channel)) = *u32_manual_command_queue;

    // Filled with the 2nd. row addr
    FTNANDC023_32BIT(Command_Queue2(u8_channel)) = *(u32_manual_command_queue + 1);

    // Filled with the 3rd. row addr
    FTNANDC023_32BIT(Command_Queue3(u8_channel)) = *(u32_manual_command_queue + 2);

    // Filled with the 4th. row addr
    FTNANDC023_32BIT(Command_Queue4(u8_channel)) = *(u32_manual_command_queue + 3);

    // Filled with the column addr and sector cnt.
    FTNANDC023_32BIT(Command_Queue5(u8_channel)) = *(u32_manual_command_queue + 4);

    return 1;
}

/*
	Function Name	: Manual_command_feature_setting_and_fire
	
	Effect 			: Setting the feature of manual command for specified channel. 

	Parameter		:
		u8_channel			--> Channel ID 
		u32_manual_feature		--> The content of sixth word in command queue
	Return value	:	
		1	--> Function success
		0	--> Function fail 
*/
BOOL Manual_command_feature_setting_and_fire(UINT8 u8_channel, UINT32 u32_manual_feature)
{
    FTNANDC023_32BIT(Command_Queue6(u8_channel)) = u32_manual_feature;
    return 1;
}

/* offset 0x24, ecc_err_fail_chx for DATA part
 */
BOOL ECC_correction_failed(UINT8 u8_channel)
{
    if (FTNANDC023_8BIT(ECC_Interrupt_Status_For_Data) & (0x1 << u8_channel)) {
        FTNANDC023_8BIT(ECC_Interrupt_Status_For_Data) |= (0x1 << u8_channel);
        return 1;
    } else {
        return 0;
    }
}

/* 0x154, interrupt status, R/W1C */
UINT8 Command_complete(UINT8 u8_channel)
{
    UINT8 u8_command_complete;

    u8_command_complete = FTNANDC023_8BIT(NANDC_Interrupt_Status + 2);
    if (u8_command_complete & (0x1 << u8_channel)) {
        FTNANDC023_8BIT(NANDC_Interrupt_Status + 2) |= (0x1 << u8_channel);
        return 1;
    }
    NANDC_DEBUG1("<%s:not>", __FUNCTION__);
    return 0;
}

/* offset 0x24, ecc_err_hit_thres_chx for DATA part
 */
BOOL ECC_failed_hit_threshold(UINT8 u8_channel)
{
    UINT8 u8_ecc_hit_threshold_for_data;

    u8_ecc_hit_threshold_for_data = FTNANDC023_8BIT(ECC_Interrupt_Status_For_Data + 1);
    if (u8_ecc_hit_threshold_for_data & (0x1 << u8_channel)) {
        FTNANDC023_8BIT(ECC_Interrupt_Status_For_Data + 1) |= (0x1 << u8_channel);
        return 1;
    }

    return 0;
}

/* 0x154, interrupt status, R/W1C */
UINT8 CRC_failed(UINT8 u8_channel)
{
    UINT8 u8_crc_failed;

    u8_crc_failed = FTNANDC023_8BIT(NANDC_Interrupt_Status + 1);
    if (u8_crc_failed & (0x1 << u8_channel)) {
        FTNANDC023_8BIT(NANDC_Interrupt_Status + 1) |= (0x1 << u8_channel);
        return 1;
    }

    return 0;
}

/* 0x154, interrupt status, R/W1C */
UINT8 Status_failed(UINT8 u8_channel)
{
    UINT8 u8_status_failed;

    u8_status_failed = FTNANDC023_8BIT(NANDC_Interrupt_Status);
    if (u8_status_failed & (0x1 << u8_channel)) {
        FTNANDC023_8BIT(NANDC_Interrupt_Status) |= (0x1 << u8_channel);
        return 1;
    }

    return 0;
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

BOOL Check_command_status(UINT8 u8_channel, UINT8 detect, unsigned char *buf)
{
    UINT8 u8_error_record = 0;
    int i, data;

    // Poll the the status
    // The command is finished only in two situations:
    // 1. Command complete
    // 2. ECC failed and the ECC mask isn't enable. (It means the command complete doesnt' be triggered.) 
    NANDC_DEBUG3("Check_command_status\n");

    while (!(Command_complete(u8_channel))) ;
    NANDC_DEBUG3("Check_command_status exit\n");
    if ((ECC_failed_hit_threshold(u8_channel)) == 1) {
        printf("MSG: Threshold Bits hit\n");
    }
#if SPARE_PAGE_MODE_SUPPORT
    if ((ECC_failed_hit_threshold_for_spare(u8_channel)) == 1) {
        printf("MSG: Threshold Bits hit for spare\n");
    }
#endif
    if (detect) {//only read need to check it
        if ((CRC_failed(u8_channel)) == 1) {
        	//printf("CRC err parsing\n");
        	for (i = 0; i < (2048 >> 2); i++) {
        		data = *(int *)buf;
        		if (data != 0xFFFFFFFF) {
            		printf("MSG: CRC checking is failed\n");
            		u8_error_record = 1;
            		break;
            	}
            }
        }
        if ((ECC_correction_failed(u8_channel)) == 1) {
        	//printf("ECC err parsing\n");
        	for (i = 0; i < (2048 >> 2); i++) {
        		data = *(int *)buf;
        		if (data != 0xFFFFFFFF) {
            		printf("MSG: ECC checking is failed\n");
            		u8_error_record = 1;
            		break;
            	}
            }
        }
    }
#if SPARE_PAGE_MODE_SUPPORT
    if ((ECC_correction_failed_for_spare(u8_channel)) == 1) {
        printf("MSG: ECC correction for spare is failed\n");
        u8_error_record = 1;
    }
#endif

    if ((Status_failed(u8_channel)) == 1) {
        printf("MSG: Status is failed\n");
        u8_error_record = 1;
    }
    // It needs to clear the ECC_correction_failed when the ECC mask is enable.
    // Command complete signal will be set no matter whether the ECC correction is success when the ECC mask is enable. 
    if (ECC_mask_status(u8_channel)) {
        if ((ECC_correction_failed(u8_channel)) == 1) {
            NANDC_DEBUG2("MSG: ECC correction is failed, but it's masked\n");
        }
    }

    /* clear all interrupt status in 0x154 */
    FTNANDC023_32BIT(NANDC_Interrupt_Status) = 0xffffffff;
		write_protect(1);
    if (u8_error_record == 1) {
        printf("MSG: Check_command_status: false ");
        return 0;
    }

    NANDC_DEBUG1("MSG: Check_command_status: true\n");
    return 1;
}

void init_cmd_feature_default(struct command_queue_feature *cmd_feature)
{
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
}

/*
 * u8_flash_page_size: page size in kbytes
 * u16_flash_block_size: how many pages in a block
 */
BOOL Memory_attribute_setting(UINT16 u16_spare_size_in_byte, UINT16 u16_flash_block_size,
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
    if (((FTNANDC023_32BIT(Command_Queue_Status) >> (u8_channel + 8)) & 0x1) == 1) {
        return 1;               // It's full
    } else {
        return 0;               // It's not full
    }
}

/* Offset 0x104, General Setting Register
 */
BOOL NANDC023_general_setting_for_all_channel(UINT8 u8_row_update_of_each_channel2channel,
                                              UINT8 u8_flash_write_protection,
                                              UINT8 u8_data_inverse, UINT8 u8_scrambler,
                                              UINT8 u8_device_busy_ready_status_bit_location,
                                              UINT8 u8_command_pass_failed_status_bit_location)
{
    FTNANDC023_32BIT(NANDC_General_Setting) =
        (UINT32) ((u8_device_busy_ready_status_bit_location << 12) |
                  (u8_command_pass_failed_status_bit_location << 8) |
                  (u8_row_update_of_each_channel2channel << 4) |
                  (u8_flash_write_protection << 2) | (u8_data_inverse << 1) | (u8_scrambler));
    return 1;
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

    UINT32 u32_command_queue[5] = { 0, 0, 0, 0, 0 };

    //Row_addr_cycle(Row_address_3cycle);
    //Column_addr_cycle(Column_address_2cycle);

    u32_command_queue[0] |= (u32_row_addr & 0xFFFFFF);
    u32_command_queue[0] |= (AUTO_CE_SEL << 25);
    u32_command_queue[0] |= (AUTO_CE_FACTOR << 26);
    u32_command_queue[4] |= u8_sector_offset;
    u32_command_queue[4] |= ((data_buf->u16_data_length_in_sector) << 16);
    Manual_command_queue_setting(u8_channel, u32_command_queue);

    return 1;
}

BOOL Page_write_with_spare_setting(UINT8 u8_channel, UINT32 u32_row_addr, UINT8 u8_sector_offset,
                                   struct collect_data_buf * data_buf)
{
    UINT32 u32_command_queue[5] = { 0, 0, 0, 0, 0 };

		write_protect(0);
    //Row_addr_cycle(Row_address_3cycle);
    //Column_addr_cycle(Column_address_2cycle);

    u32_command_queue[0] |= (u32_row_addr & 0xFFFFFF);
    u32_command_queue[0] |= (AUTO_CE_SEL << 25);
    u32_command_queue[0] |= (AUTO_CE_FACTOR << 26);
    u32_command_queue[4] |= u8_sector_offset;
    u32_command_queue[4] |= ((data_buf->u16_data_length_in_sector) << 16);
    Manual_command_queue_setting(u8_channel, u32_command_queue);

    return 1;
}

/* u16_command_index = opcode
 */
BOOL Setting_feature_and_fire(UINT8 u8_channel, UINT16 u16_command_index, UINT8 u8_starting_ce,
                              UINT32 u32_sixth_word)
{
    UINT32 u32_command_queue_content_for_sixth_word;

    u32_command_queue_content_for_sixth_word =
        (u8_starting_ce << 27) | (u16_command_index << 8) | u32_sixth_word;
    // Checking the command queue isn't full
    while (Command_queue_status_full(u8_channel)) ;

    Manual_command_feature_setting_and_fire(u8_channel, u32_command_queue_content_for_sixth_word);

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

        ftnand_Start_DMA(0,     //don't care  
                         (UINT32) (FTNANDC023_DATA_PORT_BASE + offset), (UINT32) data_buf->u8_data_buf, data_buf->u32_data_length_in_bytes, 0x02, 0x02, 6, 2,   //SrcCtrl, source address change : Inc/dec/fixed --> 0/1/2
                         0,     //DstCtrl, dest address change : Inc/dec/fixed --> 0/1/2
                         3, Command_Handshake_Enable);
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
    //UINT8 *u8_spare_buf;
    //UINT8 u8_total_spare_length_can_be_used_for_sector;
    UINT32 u32_i;//, u32_j, u32_k;
    UINT32 u32_spare_offset;
    UINT32 offset;
    //UINT32 u32_spare_area_index;

    NANDC_DEBUG2("%s in=0x%x,%d\n", __FUNCTION__, data_buf->u8_data_buf,
                 data_buf->u32_data_length_in_bytes);

    if (u8_data_presence == 1) {
        offset = 0;
        ftnand_Start_DMA(0,     //don't care                        
                         (UINT32) data_buf->u8_data_buf, FTNANDC023_DATA_PORT_BASE + offset, data_buf->u32_data_length_in_bytes, 0x02, 0x02, 6, 0,      //SrcCtrl, source address change : Inc/dec/fixed --> 0/1/2
                         2,     //DstCtrl, dest address change : Inc/dec/fixed --> 0/1/2
                         3, Command_Handshake_Enable);
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
            //unsigned char buffer[64];

            NANDC_DEBUG2("MSG: Data_write in sector mode");
#if 0
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
#else
            for (u32_i = 0; u32_i < 64; u32_i = u32_i + 4) {
            	FTNANDC023_32BIT(Spare_Sram_Access + u32_i) =
              		*((volatile UINT32 *)(data_buf->u8_spare_data_buf + u32_i));
           	}
#endif						           
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
    FTNANDC023_32BIT(Global_Software_Reset) = 1;
    // Wait for the global reset is complete.
    while ((FTNANDC023_32BIT(Global_Software_Reset) & 0x1) == 1) ;
    return 1;
}

/* Register 0x20, for ECC Error
 */
BOOL ECC_enable_interrupt(UINT8 u8_ecc_error_bit_hit_the_threshold, UINT8 u8_ecc_correct_failed,
                          UINT8 u8_ecc_error_bit_hit_the_threshold_for_spare,
                          UINT8 u8_ecc_correct_failed_for_spare)
{
    FTNANDC023_32BIT(ECC_Interrupt_Enable) =
        (UINT32) (((u8_ecc_error_bit_hit_the_threshold_for_spare) << 3) |
                  ((u8_ecc_correct_failed_for_spare) << 2) |
                  ((u8_ecc_error_bit_hit_the_threshold) << 1) | (u8_ecc_correct_failed));
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
#ifdef CONFIG_PLATFORM_GM8126    
    FTNANDC023_8BIT(ECC_threshold_ecc_error_num + u8_channel) &= ~0x1F;
    FTNANDC023_8BIT(ECC_threshold_ecc_error_num + u8_channel) = (u8_threshold_num - 1) & 0x1F;
#else
    FTNANDC023_8BIT(ECC_threshold_ecc_error_num + u8_channel) &= ~0x3F;
    FTNANDC023_8BIT(ECC_threshold_ecc_error_num + u8_channel) = (u8_threshold_num - 1) & 0x3F;
#endif    

    return 1;
}

/* Offset 0x18, Number of ECC Correction Capability bits register, Data part.
 */
BOOL ECC_correct_ecc_error(UINT8 u8_channel, UINT8 u8_correct_num)
{
    // Clear the specified field.
#ifdef CONFIG_PLATFORM_GM8126       
    FTNANDC023_8BIT(ECC_correct_ecc_error_num + u8_channel) &= ~0x1F;
    FTNANDC023_8BIT(ECC_correct_ecc_error_num + u8_channel) = (u8_correct_num - 1) & 0x1F;
#else
    FTNANDC023_8BIT(ECC_correct_ecc_error_num + u8_channel) &= ~0x3F;
    FTNANDC023_8BIT(ECC_correct_ecc_error_num + u8_channel) = (u8_correct_num - 1) & 0x3F;
#endif    

    return 1;
}

/* ECC 0x18, data part, for one sector
 */
BOOL NANDC023_ecc_occupy_for_sector(void)
{
    UINT8 u8_ecc_correct_bits, u8_occupy_byte;

#ifdef CONFIG_PLATFORM_GM8126 
    u8_ecc_correct_bits = (FTNANDC023_8BIT(ECC_correct_ecc_error_num) & 0x1F) + 1;
#else
    u8_ecc_correct_bits = (FTNANDC023_8BIT(ECC_correct_ecc_error_num) & 0x3F) + 1;
#endif    

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
    switch (value & 0x3) {
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
    chip->sparesize = (value >> 16) & 0x3FF;

    /* Sector size and ECC base */
    value = (nand023_drv_regread(ECC_Control) >> 16) & 0x1;
    chip->ecc_base = (value == 0x1) ? ECC_BASE_1024 : ECC_BASE_512;
    chip->sectorsize = (chip->ecc_base == ECC_BASE_1024) ? 1024 : 512;

    /* ECC capability */
#ifdef CONFIG_PLATFORM_GM8126      
    value = nand023_drv_regread(ECC_correct_ecc_error_num) & 0x1F;
#else
    value = nand023_drv_regread(ECC_correct_ecc_error_num) & 0x3F;
#endif    

    chip->ecc_capability = value + 1;

    /* status bit and command bit */
    value = nand023_drv_regread(NANDC_General_Setting);
    chip->status_bit = (value >> 12) & 0x7;
    chip->cmmand_bit = (value >> 8) & 0x7;

    /* row cycle and column cycle */
    value = nand023_drv_regread(Memory_Attribute_Setting);
    chip->row_cyc = ((value >> 13) & 0x3) + 1;
    chip->col_cyc = ((value >> 12) & 0x1) + 1;
    
    printf("NAND Row cyc = %d, Col cyc = %d\n", chip->row_cyc, chip->col_cyc);
}

/* Nandc023 init function
 * Many init function was done in ROM code. So we don't init the nandchip again.
 */
void nand023_drv_init(void)
{
#ifdef CONFIG_PLATFORM_GM8210    
    writel(readl(CONFIG_PMU_BASE + 0x5C) | (1 << 24), CONFIG_PMU_BASE + 0x5C);
#endif
    /* correction fail trigger the interrupt */
    FTNANDC023_32BIT(ECC_Interrupt_Enable) |= correct_fail_intr_enable;

    /* perform CRC check for channel 0 */
    FTNANDC023_32BIT(NANDC_General_Setting) &= ~(0x1 << 16);

    /* NANDC Interrupt Enable Register */
    FTNANDC023_32BIT(NANDC_Interrupt_Enable) |=
        ((crc_fail_intr_enable << 8) | flash_status_fail_intr_enable);

    /* clear all interrupt status in 0x154 */
    FTNANDC023_32BIT(NANDC_Interrupt_Status) = 0xffffffff;
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

    cmd_feature.Spare_size = p_buf.u8_spare_size_for_sector - 1;

    //Clear spare area to ensure read spare data is not previous written value.
    FTNANDC023_clear_specified_spare_sram(NAND_CHANNEL, &p_buf);

    // Checking the command queue isn't full
    while (Command_queue_status_full(NAND_CHANNEL)) ;

    Page_read_with_spare_setting(NAND_CHANNEL, u32_row_addr, 0, &p_buf);
    Setting_feature_and_fire(NAND_CHANNEL, Command(PAGE_READ_WITH_SPARE), Start_From_CE0,
                             *u32_sixth_word);
#if 0
    Data_read(NAND_CHANNEL, &p_buf, &flash_parm, 1, 0); //read data page
    printf("buf[0]=%x\n", pgbuf[0]);
    //Command FAILED      
    if (!Check_command_status(NAND_CHANNEL, 1)) {
        printf("MSG: check status fail in ftnandc023_read_buffer");
        //return 0;
    }
#endif
    NANDC_DEBUG1("%s exit\n", __FUNCTION__);
    //return 1;
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
        // The spare data is protected by CRC in Sector mode and CRC needs 2 bytes for each spare data of sector. 
        u16_occupy_spare_bytes =
            u16_occupy_spare_bytes /* data */  +
            (2 * flash_readable_info->u16_sector_in_page) /* spare */ ;
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
        if (u16_remaining_spare_space < (u8_spare_size * flash_readable_info->u16_sector_in_page)) {
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

    cmd_feature.Spare_size = p_buf.u8_spare_size_for_sector - 1;

    if (!NANDC023_check_spare_size(p_buf.u8_mode, p_buf.u8_spare_size_for_sector, &flash_parm)) {
        NANDC_DEBUG1("MSG: Spare size %d is too large! \n", p_buf.u8_spare_size_for_sector);
        return 0;
    }

    /* Set commandq 0-4 words */
    Page_write_with_spare_setting(NAND_CHANNEL, u32_row_addr, 0, &p_buf);
    /* Set sixth word */
    Setting_feature_and_fire(NAND_CHANNEL, Command(PAGE_WRITE_WITH_SPARE), Start_From_CE0,
                             *u32_sixth_word);
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

    u32_sixth_word = (UINT32 *)&cmd_feature;
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

    if (data_buf->u8_mode == Sector_mode)
        cmd_feature.Spare_size = data_buf->u8_spare_size_for_sector - 1;
    else
        return 0;

    //Clear spare area to ensure read spare data is not previous written value.
    FTNANDC023_clear_specified_spare_sram(NAND_CHANNEL, data_buf);

    // Checking the command queue isn't full
    while (Command_queue_status_full(NAND_CHANNEL)) ;

    Page_read_with_spare_setting(NAND_CHANNEL, u32_row_addr, u8_sector_offset, data_buf);
    Setting_feature_and_fire(NAND_CHANNEL, Command(PAGE_READ_WITH_SPARE), Start_From_CE0,
                             *u32_sixth_word);
    /* DMA starts */
    Data_read(NAND_CHANNEL, data_buf, &flash_parm, 1, 0);       //read data page

    Data_read(NAND_CHANNEL, data_buf, &flash_parm, 0, 1);       //read spare

    //Command FAILED        
    ret =
        Check_command_status(NAND_CHANNEL, 1, (unsigned char *)data_buf);
    if (check_status && !ret) {
        printf("MSG: check status fail in ftnandc023_read_page_with_spare");
        return 0;
    }

    return 1;
}

BOOL Read_flash_ID_setting(UINT8 u8_channel)
{
    UINT32 u32_command_queue[5] = { 0, 0, 0, 0, 0 };

    //Row_addr_cycle(Row_address_1cycle);

    u32_command_queue[4] = (1 << 16);
    Manual_command_queue_setting(u8_channel, u32_command_queue);
    return 1;
}

BOOL Byte_mode_read_setting(UINT8 u8_channel, UINT32 u32_row_addr, UINT16 u16_col_addr,
                            struct collect_data_buf * data_buf)
{

    UINT32 u32_command_queue[5] = { 0, 0, 0, 0, 0 };

    //Row_addr_cycle(Row_address_3cycle);
    //Column_addr_cycle(Column_address_2cycle);

    u32_command_queue[0] |= (u32_row_addr & 0xFFFFFF);
    u32_command_queue[0] |= (AUTO_CE_SEL << 25);
    u32_command_queue[0] |= (AUTO_CE_FACTOR << 26);
    u32_command_queue[4] |= u16_col_addr;
    u32_command_queue[4] |= ((data_buf->u16_data_length_in_sector) << 16);
    Manual_command_queue_setting(u8_channel, u32_command_queue);

    return 1;
}

#endif
