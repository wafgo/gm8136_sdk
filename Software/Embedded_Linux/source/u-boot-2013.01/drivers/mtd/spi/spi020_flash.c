#include <common.h>
#include "spi020_flash.h"
#include "spi_flash_internal.h"
#include <malloc.h>
#include <config.h>
#include <asm/io.h>
//#include "../../spi/ftssp010_spi.h"

static int quad_enable = -1;
//static int write_enable = -1;
static int flash_en4b = -1;

struct spi020_flash spi_flash_info = { };

NOR_SYSHDR *spi_sys_hdr;
unsigned char spi_pgbuf[0x100];

static const struct {
	const u8 shift;
	const u8 idcode;
	struct spi_flash *(*probe) (struct spi_slave *spi, u8 *idcode);
} flashes[] = {
	/* Keep it sorted by define name */
#ifdef CONFIG_SPI_FLASH_ATMEL
	{ 0, 0x1f, spi_flash_probe_atmel, },
#endif
#ifdef CONFIG_SPI_FLASH_EON
	{ 0, 0x1c, spi_flash_probe_eon, },
#endif
#ifdef CONFIG_SPI_FLASH_MACRONIX
	{ 0, 0xc2, spi_flash_probe_macronix, },
#endif
#ifdef CONFIG_SPI_FLASH_SPANSION
	{ 0, 0x01, spi_flash_probe_spansion, },
#endif
#ifdef CONFIG_SPI_FLASH_SST
	{ 0, 0xbf, spi_flash_probe_sst, },
#endif
#ifdef CONFIG_SPI_FLASH_STMICRO
	{ 0, 0x20, spi_flash_probe_stmicro, },
#endif
#ifdef CONFIG_SPI_FLASH_WINBOND
	{ 0, 0xef, spi_flash_probe_winbond, },
#endif
#ifdef CONFIG_SPI_FLASH_ESMT
	{ 0, 0x8c, spi_flash_probe_esmt, },
#endif
#ifdef CONFIG_SPI_FLASH_GD
	{ 0, 0xc8, spi_flash_probe_gd, },
#endif
#ifdef CONFIG_SPI_FRAM_RAMTRON
	{ 6, 0xc2, spi_fram_probe_ramtron, },
# undef IDCODE_CONT_LEN
# define IDCODE_CONT_LEN 6
#endif
	/* Keep it sorted by best detection */
#ifdef CONFIG_SPI_FLASH_STMICRO
	{ 0, 0xff, spi_flash_probe_stmicro, },
#endif
#ifdef CONFIG_SPI_FRAM_RAMTRON_NON_JEDEC
	{ 0, 0xff, spi_fram_probe_ramtron, },
#endif
};

#define IDCODE_LEN 4

unsigned char OPCODE_READ_COMMAND = COMMAND_FAST_READ, OPCODE_WRITE_COMMAND = COMMAND_WRITE_PAGE, OPCODE_ERASE_COMMAND = COMMAND_ERASE_64K_BLOCK;

int spi_xfer(struct spi_slave *spi, unsigned int len, const void *dout, void *din,
                unsigned long flags);
int spi_dataflash_en4b(struct spi_slave *spi, int enable);

#ifdef CONFIG_PLATFORM_GM8139
/* 4 byte mode return 1 */
int four_byte_mode(void)
{
    int ret = 0;
    
    if(readl(CONFIG_PMU_BASE) & 0xF000) {
        if (readl(CONFIG_PMU_BASE + 0x4) & BIT7)
      	    ret = 0;
      	else
      	    ret = 1;
    } else {
        if (readl(CONFIG_PMU_BASE + 0x4) & BIT7)
  	        ret = 1;
  	}
    
    return ret;
}
#endif

#ifdef CONFIG_PLATFORM_GM8136
/* 4 byte mode return 1 */
int four_byte_mode(void)
{
    int ret = 0;

    if (readl(CONFIG_PMU_BASE + 0x4) & BIT23)
        ret = 1;
    else
        ret = 0;
    
    return ret;
}
#endif
// ************************* flash releated functions *************************
void transfer_param(struct spi_flash *flash)
{
    spi_flash_info.ce = 0;
    //printf("SF%d,name=%s\n", params->l2_page_size,params->name);
    /* winbond read status command is dirrerence with other chip */

	//printf("transfer_param=0x%x,%d,%d\n",flash->size,flash->page_size,flash->sector_size);
    spi_flash_info.page_size = flash->page_size;
    spi_flash_info.size = flash->size;
    spi_flash_info.erase_sector_size = flash->sector_size;
    spi_flash_info.nr_pages = flash->size / flash->sector_size;
    
    if(flash->size > 0x1000000){
        spi_flash_info.mode_3_4 = 4;
        spi_dataflash_en4b(flash->spi, 1);
    } else {
        spi_flash_info.mode_3_4 = 3;
    }
    
    printf("flash is %dbyte mode\n", spi_flash_info.mode_3_4);

    if (spi_flash_info.mode_3_4 == 3){
        OPCODE_READ_COMMAND = COMMAND_FAST_READ;
        OPCODE_WRITE_COMMAND = COMMAND_WRITE_PAGE;
        OPCODE_ERASE_COMMAND = COMMAND_ERASE_64K_BLOCK;         
    } else {
        switch (flash->name[0]){
            case 'W':
                spi_flash_info.chip_type = 1;
                printf("WINBOND chip\n");
                OPCODE_READ_COMMAND = COMMAND_FAST_READ;
                OPCODE_WRITE_COMMAND = COMMAND_WRITE_PAGE;
                OPCODE_ERASE_COMMAND = COMMAND_ERASE_64K_BLOCK; 
                break;
            case 'M':
                spi_flash_info.chip_type = 2;
                printf("MXIC chip\n");
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
                if (four_byte_mode()) {
                    OPCODE_READ_COMMAND = COMMAND_FAST_READ;
                    OPCODE_WRITE_COMMAND = COMMAND_WRITE_PAGE;
                    OPCODE_ERASE_COMMAND = COMMAND_ERASE_64K_BLOCK;
                } else {
                    OPCODE_READ_COMMAND = COMMAND_FAST_READ_0x0C;
                    OPCODE_WRITE_COMMAND = COMMAND_WRITE_PAGE_0x12;
                    OPCODE_ERASE_COMMAND = COMMAND_ERASE_64K_BLOCK_0xDC;                    
                }
#else
                OPCODE_READ_COMMAND = COMMAND_FAST_READ;
                OPCODE_WRITE_COMMAND = COMMAND_WRITE_PAGE;
                OPCODE_ERASE_COMMAND = COMMAND_ERASE_64K_BLOCK;              
#endif                
                break;

            case 'E':
                spi_flash_info.chip_type = 3;
                printf("EON chip\n");
                OPCODE_READ_COMMAND = COMMAND_FAST_READ;
                OPCODE_WRITE_COMMAND = COMMAND_WRITE_PAGE;
                OPCODE_ERASE_COMMAND = COMMAND_ERASE_64K_BLOCK;             
                break;
            case 'G':
                spi_flash_info.chip_type = 3;
                printf("GD chip\n");
                OPCODE_READ_COMMAND = COMMAND_FAST_READ;
                OPCODE_WRITE_COMMAND = COMMAND_WRITE_PAGE;
                OPCODE_ERASE_COMMAND = COMMAND_ERASE_64K_BLOCK;             
                break;                
            case 'S':
                spi_flash_info.chip_type = 2;
                printf("SPANSION chip\n");
                OPCODE_READ_COMMAND = COMMAND_FAST_READ;
                OPCODE_WRITE_COMMAND = COMMAND_WRITE_PAGE;
                OPCODE_ERASE_COMMAND = COMMAND_ERASE_64K_BLOCK;
                break;                
            default:
                spi_flash_info.chip_type = 0;
                printf("unknow chip\n");
                OPCODE_READ_COMMAND = COMMAND_FAST_READ;
                OPCODE_WRITE_COMMAND = COMMAND_WRITE_PAGE;
                OPCODE_ERASE_COMMAND = COMMAND_ERASE_64K_BLOCK;            
                break;
        }        
    }  
}

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
			unsigned int max_hz, unsigned int mode)
{
	struct spi_slave *fs = NULL;

	if(bus != 0){
		printf("only support bus 0\n");
		return NULL;
	}	
	fs = malloc(sizeof(struct spi_slave));
	if (!fs){
		return NULL;
	}

	fs->bus = bus;
	fs->cs = cs;

	return fs;
}

void spi_free_slave(struct spi_slave *slave)
{
	free(slave);
}

int spi_claim_bus(struct spi_slave *slave)
{
	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
}

int spi020_flash_cmd(struct spi_slave *spi, UINT8 * u8_cmd, void *response, size_t len)
{
    int ret;

    ret = spi_xfer(spi, len, u8_cmd, NULL, SPI_XFER_CMD_STATE | SPI_XFER_CHECK_CMD_COMPLETE);
    if (ret) {
        printf("SF: Failed to send command %02x: %d\n", u8_cmd[0], ret);
        return ret;
    }

    if (len && response != NULL) {
        //printf("A1\n");
        ret = spi_xfer(spi, len, NULL, response, SPI_XFER_DATA_IN_STATE);
        if (ret) {
            printf("SF: Failed to read response (%zu bytes): %d\n", len, ret);
        }
    } else if ((len && response == NULL) || (!len && response != NULL)) {
        printf
            ("SF: Failed to read response due to the mismatch of len and response (%zu bytes): %d\n",
             len, ret);
    }

    return ret;
}

int spi020_flash_cmd_write(struct spi_slave *spi, UINT8 * u8_cmd, const void *data, int data_len)
{
    int ret;

    ret = spi_xfer(spi, data_len, u8_cmd, NULL, SPI_XFER_CMD_STATE);
    if (ret) {
        printf("SF: Failed to send command %02x: %d\n", u8_cmd[0], ret);
        return ret;
    } else if (data_len != 0) {
        ret =
            spi_xfer(spi, data_len, data, NULL,
                        SPI_XFER_DATA_OUT_STATE | SPI_XFER_CHECK_CMD_COMPLETE);
        if (ret) {
            printf("SF: Failed to write data (%zu bytes): %d\n", data_len, ret);
        }
    }

    return ret;
}

int spi020_flash_cmd_read(struct spi_slave *spi, UINT8 * u8_cmd, void *data, int data_len)
{
    int ret = -1;

    ret = spi_xfer(spi, data_len, u8_cmd, NULL, SPI_XFER_CMD_STATE);
//printf("spi020_flash_cmd_read cmd=0x%x,len=0x%x\n",*u8_cmd,data_len);    
    if (ret) {
        printf("SF: Failed to send command %02x: %d\n", u8_cmd[0], ret);
        return ret;
    } else if (data_len != 0) {
        //printf("A0\n");
        ret =
            spi_xfer(spi, data_len, NULL, data,
                        SPI_XFER_DATA_IN_STATE | SPI_XFER_CHECK_CMD_COMPLETE);
        if (ret) {
            printf("SF: Failed to read data (%zu bytes): %d\n", data_len, ret);
        }
    }

    return ret;
}

/* Wait until BUSY bit clear
 */
void flash_wait_busy_ready(struct spi_slave *spi)
{
    UINT8 rd_sts_cmd[1];
    UINT8 status;

    rd_sts_cmd[0] = COMMAND_READ_STATUS1;

    do {
        if (spi020_flash_cmd(spi, rd_sts_cmd, NULL, 0)) {
            printf("Failed to check status!\n");
            break;
        }

        ftspi020_read_status(&status);
    } while (status & FLASH_STS_BUSY);
}

#ifdef CONFIG_SPI_QUAD
static int flash_set_quad_enable(struct spi_slave *spi, int en)
{
    UINT8 rd_sts_cmd[1];
    UINT8 wr_sts_cmd[1];
    UINT8 status[2];

    /* do nothing if quad enabled/disabled already */
    if (quad_enable == en)
        return 0;

    quad_enable = en;

    if (spi_flash_info.chip_type) {
        /* read the status from Status Register-1 
         */
        printf("status register two bytes\n");
        rd_sts_cmd[0] = COMMAND_READ_STATUS1;
        if (spi020_flash_cmd(spi, rd_sts_cmd, NULL, 0)) {
            printf("flash_set_quad_enable fail 1\n");
            return -1;
        }
        ftspi020_read_status(&status[0]);

        /* read the status from Status Register-2 
         */
        rd_sts_cmd[0] = COMMAND_READ_STATUS2;
        if (spi020_flash_cmd(spi, rd_sts_cmd, NULL, 0)) {
            printf("flash_set_quad_enable fail 2\n");
            return -1;
        }
        ftspi020_read_status(&status[1]);

        wr_sts_cmd[0] = COMMAND_WRITE_ENABLE;
        if (spi020_flash_cmd(spi, wr_sts_cmd, NULL, 0)) {
            printf("flash_set_quad_enable fail 3\n");
            return -1;
        }

        if (en)
            status[1] |= FLASH_WINBOND_STS_QUAD_ENABLE;
        else
            status[1] &= ~FLASH_WINBOND_STS_QUAD_ENABLE;

        wr_sts_cmd[0] = COMMAND_WRITE_STATUS;
        if (spi020_flash_cmd_write(spi, wr_sts_cmd, (const void *)status, 2)) {
            printf("flash_set_quad_enable fail 4\n");
            return -1;
        }

        /* check again */
        rd_sts_cmd[0] = COMMAND_READ_STATUS2;
        if (spi020_flash_cmd(spi, rd_sts_cmd, NULL, 0)) {
            printf("flash_set_quad_enable fail 5 \n");
            return -1;
        }
        ftspi020_read_status(&status[1]);

        /* QE which is S9 of Status Register-2 */
        if ((status[1] & FLASH_WINBOND_STS_QUAD_ENABLE) != 0) {
            printf("flash_set_quad_enable fail 6! \n");
            return -1;
        }
    } else {
        /* read the status from Status Register-1 
         */
        printf("status register one byte\n");
        rd_sts_cmd[0] = COMMAND_READ_STATUS1;
        if (spi020_flash_cmd(spi, rd_sts_cmd, NULL, 0)) {
            printf("flash_set_quad_enable fail 1\n");
            return -1;
        }
        ftspi020_read_status(&status[0]);

        wr_sts_cmd[0] = COMMAND_WRITE_ENABLE;
        if (spi020_flash_cmd(spi, wr_sts_cmd, NULL, 0)) {
            printf("flash_set_quad_enable fail 2\n");
            return -1;
        }

        if (en)
            status[0] |= FLASH_STS_QUAD_ENABLE;
        else
            status[0] &= ~FLASH_STS_QUAD_ENABLE;

        wr_sts_cmd[0] = COMMAND_WRITE_STATUS;
        if (spi020_flash_cmd_write(spi, wr_sts_cmd, (const void *)status, 1)) {
            printf("flash_set_quad_enable fail 3\n");
            return -1;
        }

        /* check again */
        rd_sts_cmd[0] = COMMAND_READ_STATUS1;
        if (spi020_flash_cmd(spi, rd_sts_cmd, NULL, 0)) {
            printf("flash_set_quad_enable fail 4 \n");
            return -1;
        }

        ftspi020_read_status(&status[0]);

        /* QE which is S9 of Status Register-1 */
        if ((status[1] & FLASH_STS_QUAD_ENABLE) != 0) {
            printf("flash_set_quad_enable fail 5! \n");
            return -1;
        }
    }

    return 0;
}
#endif

int spi_xfer(struct spi_slave *spi, unsigned int len, const void *dout, void *din,
                unsigned long flags)
{
    struct ftspi020_cmd spi_cmd;
    UINT8 *u8_data_out = (UINT8 *) dout;

    memset(&spi_cmd, 0, sizeof(struct ftspi020_cmd));

    /* send the instruction */
    if (flags & SPI_XFER_CMD_STATE) {
        spi_cmd.start_ce = spi->cs;
        spi_cmd.ins_code = *u8_data_out;
        //printf("dout=0x%x,din=0x%x,%d,%d\n",*(UINT8 *)dout,*(UINT8 *)din, len, spi->cs);
        spi_cmd.intr_en = cmd_complete_intr_enable;
#ifdef CONFIG_CMD_FPGA
        spi_cmd.start_ce = 0;
#endif
        switch (spi_cmd.ins_code) {
        case CMD_READ_ID:
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.write_en = spi_read;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            spi_cmd.data_cnt = IDCODE_LEN;
            break;
        case COMMAND_RESET:
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.write_en = spi_write;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            break;
        case COMMAND_WRITE_ENABLE:
        case COMMAND_WRITE_DISABLE:
        case COMMAND_EN4B:
        case COMMAND_EX4B:
            //if (write_enable == spi_cmd.ins_code){
            //          printf("exit\n");
            //    goto exit;
            //  }
            //write_enable = spi_cmd.ins_code;
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.write_en = spi_write;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            break;
        case COMMAND_ERASE_SECTOR:     //erase 4K
        case COMMAND_ERASE_64K_BLOCK:
        case COMMAND_ERASE_64K_BLOCK_0xDC:
            if (spi_flash_info.mode_3_4 == 4) {
                spi_cmd.spi_addr =
                    (*(u8_data_out + 4) << 24) | (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) <<
                                                                               8) | (*(u8_data_out +
                                                                                       1));
                spi_cmd.addr_len = addr_4byte;
            } else {
                spi_cmd.spi_addr =
                    (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
                spi_cmd.addr_len = addr_3byte;
            }
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.write_en = spi_write;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            break;
        case COMMAND_ERASE_CHIP:
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.write_en = spi_write;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            break;
        case COMMAND_WRITE_STATUS:
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.write_en = spi_write;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            spi_cmd.data_cnt = len;
            break;
        case COMMAND_READ_STATUS1:
        case COMMAND_READ_STATUS2:
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.write_en = spi_read;
            spi_cmd.read_status_en = read_status_enable;
            spi_cmd.read_status = read_status_by_sw;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            break;
        case COMMAND_WRITE_PAGE:
        case COMMAND_WRITE_PAGE_0x12:
            if (spi_flash_info.mode_3_4 == 4) {
                spi_cmd.spi_addr =
                    (*(u8_data_out + 4) << 24) | (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) <<
                                                                               8) | (*(u8_data_out +
                                                                                       1));
                spi_cmd.addr_len = addr_4byte;
            } else {
                spi_cmd.spi_addr =
                    (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
                spi_cmd.addr_len = addr_3byte;
            }
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.data_cnt = len;
            spi_cmd.write_en = spi_write;
            spi_cmd.read_status_en = read_status_disable;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            break;
        case COMMAND_WINBOND_QUAD_WRITE_PAGE:
        case COMMAND_QUAD_WRITE_PAGE:
            if (spi_flash_info.mode_3_4 == 4) {
                spi_cmd.spi_addr =
                    (*(u8_data_out + 4) << 24) | (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) <<
                                                                               8) | (*(u8_data_out +
                                                                                       1));
                spi_cmd.addr_len = addr_4byte;
            } else {
                spi_cmd.spi_addr =
                    (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
                spi_cmd.addr_len = addr_3byte;
            }
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.data_cnt = len;
            spi_cmd.write_en = spi_write;
            spi_cmd.read_status_en = read_status_disable;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_quad_mode;
            break;
        case COMMAND_FAST_READ_QUAD_IO:
            if (spi_flash_info.mode_3_4 == 4) {
                spi_cmd.spi_addr =
                    (*(u8_data_out + 4) << 24) | (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) <<
                                                                               8) | (*(u8_data_out +
                                                                                       1));
                spi_cmd.addr_len = addr_4byte;
            } else {
                spi_cmd.spi_addr =
                    (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
                spi_cmd.addr_len = addr_3byte;
            }
            spi_cmd.dum_2nd_cyc = 4;
            spi_cmd.conti_read_mode_en = 1;
            spi_cmd.conti_read_mode_code = 0;
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.data_cnt = len;
            spi_cmd.write_en = spi_read;
            spi_cmd.read_status_en = read_status_disable;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_quadio_mode;
            break;
        case COMMAND_FAST_READ:
        case COMMAND_FAST_READ_0x0C:
        case COMMAND_FAST_READ_0x1B:
            if (spi_flash_info.mode_3_4 == 4) {
                spi_cmd.spi_addr =
                    (*(u8_data_out + 4) << 24) | (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) <<
                                                                               8) | (*(u8_data_out +
                                                                                       1));
                spi_cmd.addr_len = addr_4byte;
            } else {
                spi_cmd.spi_addr =
                    (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
                spi_cmd.addr_len = addr_3byte;
            }
            spi_cmd.dum_2nd_cyc = 8;
            spi_cmd.conti_read_mode_en = 0;
            spi_cmd.conti_read_mode_code = 0;
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.data_cnt = len;
            spi_cmd.write_en = spi_read;
            spi_cmd.read_status_en = read_status_disable;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            break;
        default:
            printf("Not define this command 0x%x!!!!!!!\n", spi_cmd.ins_code);
            goto xfer_exit;
            break;
        }
        /* sent the command out */
        ftspi020_issue_cmd(&spi_cmd);

    } else if (flags & SPI_XFER_DATA_IN_STATE) {
        /* read the data */
        //printf("\n");
        ftspi020_data_access(1, (UINT8 *) din, len);
    } else if (flags & SPI_XFER_DATA_OUT_STATE) {
        /* send the data */
        ftspi020_data_access(0, (UINT8 *) dout, len);
    }

    /* check command complete */
    if (flags & SPI_XFER_CHECK_CMD_COMPLETE)
        ftspi020_wait_cmd_complete();

xfer_exit:    
    return 0;
}

int spi_flash_cmd_read_fast(struct spi_flash *flash, u32 offset, size_t len, void *buf)
{
    int ret;
    UINT8 u8_rd_cmd[5];
    
    //printf("\n");
    /* check if the flash is busy */
    flash_wait_busy_ready(flash->spi);

#ifdef CONFIG_SPI_QUAD
    flash_set_quad_enable(flash->spi, 1);
    u8_rd_cmd[0] = COMMAND_FAST_READ_QUAD_IO;
#else
    u8_rd_cmd[0] = OPCODE_READ_COMMAND;
#endif
    u8_rd_cmd[1] = offset & 0xFF;
    u8_rd_cmd[2] = ((offset & 0xFF00) >> 8);
    u8_rd_cmd[3] = ((offset & 0xFF0000) >> 16);
    if (spi_flash_info.mode_3_4 == 4) {
        u8_rd_cmd[4] = ((offset & 0xFF000000) >> 24);
//        spi_dataflash_en4b(flash->spi, 1);
    }

    ret = spi020_flash_cmd_read(flash->spi, u8_rd_cmd, buf, len);

    //if (spi_flash_info.mode_3_4 == 4)
    //    spi_dataflash_en4b(flash->spi, 0);

    return ret;
}

/* offset: byte offset
 * len: how many bytes.
 */
int spi_flash_cmd_write_multi(struct spi_flash *flash, u32 offset, size_t len, const void *buf)
{
    int ret;
    int start_page, offset_in_start_page, len_each_times;
    UINT8 u8_wr_en_cmd[1], u8_wr_cmd[5];
    UINT8 *u8_buf = (UINT8 *)buf, *alignment_buf;

    alignment_buf = (UINT8 *) malloc(spi_flash_info.page_size);
    if (alignment_buf == NULL) {
        printf("No buffer in spi_dataflash_write_fast !\n");
        return -1;
    }

    start_page = offset / (spi_flash_info.page_size);
    offset_in_start_page = offset % (spi_flash_info.page_size);

    /*
       This judgement, "if(len + offset_in_start_page <= flash->page_size)"
       is for the case of (offset + len) doesn't exceed the nearest next page boundary. 
       0                                255
       -------------------------------------
       | | | | |.......................| | |
       -------------------------------------
       256                               511                                
       -------------------------------------
       | |*|*|*|.......................|*| |
       -------------------------------------
       512                               767
       -------------------------------------
       | | | | |.......................| | |
       -------------------------------------
     */
    if (len + offset_in_start_page <= spi_flash_info.page_size) {
        len_each_times = len;
    } else {
        len_each_times = ((((start_page + 1) * spi_flash_info.page_size) - 1) - offset) + 1;
    }

    /* check if the flash is busy */
    flash_wait_busy_ready(flash->spi);

    //if (spi_flash_info.mode_3_4 == 4)
    //    spi_dataflash_en4b(flash->spi, 1);

#ifdef CONFIG_SPI_QUAD
    ret = flash_set_quad_enable(flash->spi, 1);
    if (ret) {
        ret = -1;
        goto exit;
    }
#endif

    u8_wr_en_cmd[0] = COMMAND_WRITE_ENABLE;
    if (spi020_flash_cmd(flash->spi, u8_wr_en_cmd, NULL, 0)) {
        ret = -1;
        goto exit;
    }
#ifdef CONFIG_SPI_QUAD
    //only for QUAD_PROGRAM
    if (spi_flash_info.chip_type)
        u8_wr_cmd[0] = COMMAND_WINBOND_QUAD_WRITE_PAGE;
    else
        u8_wr_cmd[0] = COMMAND_QUAD_WRITE_PAGE;
#else
    u8_wr_cmd[0] = OPCODE_WRITE_COMMAND;
#endif

    u8_wr_cmd[1] = offset & 0xFF;
    u8_wr_cmd[2] = ((offset & 0xFF00) >> 8);
    u8_wr_cmd[3] = ((offset & 0xFF0000) >> 16);
    if (spi_flash_info.mode_3_4 == 4)
        u8_wr_cmd[4] = ((offset & 0xFF000000) >> 24);

    if (spi020_flash_cmd_write(flash->spi, u8_wr_cmd, u8_buf, len_each_times)) {
        ret = -1;
        goto exit;
    }

    u8_buf = u8_buf + len_each_times;

    do {
        // To avoid the "u8_buf" isn't alignment. 
        memcpy(alignment_buf, u8_buf, spi_flash_info.page_size);

        len = len - len_each_times;
        offset = ((offset / spi_flash_info.page_size) + 1) * spi_flash_info.page_size;
        if (len < (spi_flash_info.page_size)) {
            if (len == 0) {
                break;
            } else {
                len_each_times = len;
            }
        } else {
            len_each_times = spi_flash_info.page_size;
        }

        /* check if the flash is busy */
        flash_wait_busy_ready(flash->spi);

        u8_wr_en_cmd[0] = COMMAND_WRITE_ENABLE;
        if (spi020_flash_cmd(flash->spi, u8_wr_en_cmd, NULL, 0)) {
            ret = -1;
            goto exit;
        }
#ifdef CONFIG_SPI_QUAD
        //only for QUAD_PROGRAM
        u8_wr_cmd[0] = COMMAND_QUAD_WRITE_PAGE;
#else
        u8_wr_cmd[0] = OPCODE_WRITE_COMMAND;
#endif
        u8_wr_cmd[1] = offset & 0xFF;
        u8_wr_cmd[2] = ((offset & 0xFF00) >> 8);
        u8_wr_cmd[3] = ((offset & 0xFF0000) >> 16);
        if (spi_flash_info.mode_3_4 == 4)
            u8_wr_cmd[4] = ((offset & 0xFF000000) >> 24);

        //      fLib_printf("offset:0x%x len:0x%x\n", offset, len_each_times);
        if (spi020_flash_cmd_write(flash->spi, u8_wr_cmd, alignment_buf, len_each_times)) {
            ret = -1;
            goto exit;
        }
        u8_buf = u8_buf + len_each_times;
    } while (1);

    ret = 0;
exit:
    //if (spi_flash_info.mode_3_4 == 4)
    //    spi_dataflash_en4b(flash->spi, 0);

    free(alignment_buf);

    return ret;
}

/* offset: byte offset
 * len: how many bytes.
 */
int spi_flash_cmd_erase(struct spi_flash *flash, u32 offset, size_t len)
{
    int ret, addr, erase_size;
    UINT8 u8_er_cmd[5];
    UINT8 cmd_code;

    //only for SECTOR_ERASE    
    erase_size = spi_flash_info.erase_sector_size;
    if(erase_size == 0x1000) {//erase 4k
        cmd_code = COMMAND_ERASE_SECTOR;
        printf("Erase command 4K\n");
    }else{
        cmd_code = OPCODE_ERASE_COMMAND;
        printf("Erase command 64K\n");
    }
              
    offset &= ~(erase_size - 1);

    /* check if the flash is busy */
    flash_wait_busy_ready(flash->spi);

    //if (spi_flash_info.mode_3_4 == 4)
    //    spi_dataflash_en4b(flash->spi, 1);

    for (addr = offset; addr < (offset + len); addr += erase_size) {
        u8_er_cmd[0] = COMMAND_WRITE_ENABLE;
        ret = spi020_flash_cmd(flash->spi, u8_er_cmd, NULL, 0);
        if (ret)
            break;

        u8_er_cmd[0] = cmd_code;
        u8_er_cmd[1] = addr & 0xFF;
        u8_er_cmd[2] = ((addr & 0xFF00) >> 8);
        u8_er_cmd[3] = ((addr & 0xFF0000) >> 16);
        if (spi_flash_info.mode_3_4 == 4)
            u8_er_cmd[4] = ((addr & 0xFF000000) >> 24);

        ret = spi020_flash_cmd(flash->spi, u8_er_cmd, NULL, 0);
        if (ret)
            break;

        /* check if the flash is busy */
        flash_wait_busy_ready(flash->spi);
    }
    //if (spi_flash_info.mode_3_4 == 4)
    //    spi_dataflash_en4b(flash->spi, 0);

    return ret;
}

/* enable: 1 for 4 byte mode, 0 for exit 4 byte mode 
 */
int spi_dataflash_en4b(struct spi_slave *spi, int enable)
{
#if 0
    return 0;//not deal with 3 byte / 4 byte mode switch
#else    
    UINT8 u8_wr_en_cmd[1];
    int ret = 0;

    if (flash_en4b == enable)
        return 0;

    flash_en4b = enable;

    /* check if the flash is busy */
    flash_wait_busy_ready(spi);

    if (enable)
        u8_wr_en_cmd[0] = COMMAND_EN4B;
    else
        u8_wr_en_cmd[0] = COMMAND_EX4B;

    if (spi020_flash_cmd(spi, u8_wr_en_cmd, NULL, 0)) {
        ret = -1;
    }

    return ret;
#endif    
}

struct spi_flash *spi_flash_probe(unsigned int bus, unsigned int cs,
		unsigned int max_hz, unsigned int spi_mode)
{
	struct spi_slave *spi;
	struct spi_flash *flash = NULL;
	int ret, i, shift;
	u8 idcode[IDCODE_LEN + 16], *idp;

    spi_init();

	spi = spi_setup_slave(bus, cs, max_hz, spi_mode);
	if (!spi) {
		printf("SF: Failed to set up slave\n");
		return NULL;
	}

	ret = spi_claim_bus(spi);
	if (ret) {
		debug("SF: Failed to claim SPI bus: %d\n", ret);
		goto err_claim_bus;
	}

	/* Read the ID codes */
	if(bus == 0){
		ftspi020_cmd_t  spi_cmd;
		//printf("A3\n");
		ftspi020_setup_cmdq(&spi_cmd, CMD_READ_ID, instr_1byte, spi_read, spi_operate_serial_mode, IDCODE_LEN);		
		ftspi020_wait_cmd_complete();		
		ftspi020_data_access(1, idcode, sizeof(idcode));
	}else{
	    printf("bus not equal 0\n");
	    goto err_read_id;
	}
#if 1//def DEBUG
	printf("SF: Got idcodes\n");
	print_buffer(0, idcode, 1, IDCODE_LEN, 0);
#endif

	/* count the number of continuation bytes */
	for (shift = 0, idp = idcode;
	     shift < IDCODE_LEN && *idp == 0x7f;
	     ++shift, ++idp)
		continue;

	/* search the table for matches in shift and id */
	for (i = 0; i < ARRAY_SIZE(flashes); ++i)
		if (flashes[i].shift == shift && flashes[i].idcode == *idp) {
			/* we have a match, call probe */
			flash = flashes[i].probe(spi, idp);
			if (flash)
				break;
		}

	if (!flash) {
		printf("SF: Unsupported manufacturer %02x\n", *idp);
		goto err_manufacturer_probe;
	}
	else
	{
	    spi_flash_cmd_read_fast(flash, 0, 0x100, spi_pgbuf);
	    spi_sys_hdr = (NOR_SYSHDR *) spi_pgbuf;
    }
    
	printf("SF: Detected %s with page size ", flash->name);
	print_size(flash->sector_size, ", total ");
	print_size(flash->size, "\n");

    transfer_param(flash);
	spi_release_bus(spi);

	return flash;

err_manufacturer_probe:
err_read_id:
	spi_release_bus(spi);
err_claim_bus:
	spi_free_slave(spi);
	return NULL;
}

void spi_flash_free(struct spi_flash *flash)
{
	spi_free_slave(flash->spi);
	free(flash);
}

static int spi_flash_read_write(struct spi_slave *spi,
				const u8 *cmd, size_t cmd_len,
				const u8 *data_out, u8 *data_in,
				size_t data_len)
{
	unsigned long flags = SPI_XFER_BEGIN;
	int ret;

	if (data_len == 0)
		flags |= (SPI_XFER_END | SPI_XFER_CHECK_CMD_COMPLETE);

	ret = spi_xfer(spi, cmd_len * 8, cmd, NULL, flags);
	if (ret) {
		debug("SF: Failed to send command (%zu bytes): %d\n",
				cmd_len, ret);
	} else if (data_len != 0) {
	    if(data_in == NULL) {// write
    		if(data_len % 4){
    		    if(*cmd != CMD_WRITE_STATUS)
    			    printf("data len %x not 4 times\n",data_len);
    			return ret;
    		}
	    }
	    ret = spi_xfer(spi, data_len * 8, data_out, data_in, SPI_XFER_CHECK_CMD_COMPLETE);//SPI_XFER_END, 0);
        if (ret)
			debug("SF: Failed to transfer %zu bytes of data: %d\n",
					data_len, ret);
	}

	return ret;
}

int spi_flash_cmd(struct spi_slave *spi, u8 cmd, void *response, size_t len)
{
	return spi_flash_cmd_read(spi, &cmd, 1, response, len);
}

int spi_flash_cmd_read(struct spi_slave *spi, const u8 *cmd,
		size_t cmd_len, void *data, size_t data_len)
{
	return spi_flash_read_write(spi, cmd, cmd_len, NULL, data, data_len);
}

int spi_flash_cmd_write(struct spi_slave *spi, const u8 *cmd, size_t cmd_len,
		const void *data, size_t data_len)
{
	return spi_flash_read_write(spi, cmd, cmd_len, data, NULL, data_len);
}

int spi_flash_cmd_write_status(struct spi_flash *flash, u8 sr)
{
	u8 cmd;
	int ret;

	ret = spi_flash_cmd_write_enable(flash);
	if (ret < 0) {
		debug("SF: enabling write failed\n");
		return ret;
	}

	cmd = CMD_WRITE_STATUS;
	ret = spi_flash_cmd_write(flash->spi, &cmd, 1, &sr, 1);
	if (ret) {
		debug("SF: fail to write status register\n");
		return ret;
	}
	
	return 0;
}
