#include <stdio.h>
#include <string.h>
#include "spi020.h"
#include "spi_flash.h"
#include "kprint.h"

extern FLASH_TYPE_T flash_type;

static int write_enable = -1;
static int flash_en4b = -1;
// ************************* flash releated functions *************************
int spi_flash_cmd(struct spi_flash *slave, UINT8 * u8_cmd, void *response, size_t len, int sts_hw)
{
    int ret;

    ret =
        slave->spi_xfer(slave, len, u8_cmd, NULL, SPI_XFER_CMD_STATE | SPI_XFER_CHECK_CMD_COMPLETE,
                        sts_hw);
    if (ret) {
        KPrint("SF: Failed to send command %02x: %d\n", u8_cmd[0], ret);
        return ret;
    }

    if (len && response != NULL) {
        ret = slave->spi_xfer(slave, len, NULL, response, SPI_XFER_DATA_STATE, sts_hw);
        if (ret) {
            KPrint("SF: Failed to read response (%zu bytes): %d\n", len, ret);
        }
    } else if ((len && response == NULL) || (!len && response != NULL)) {
        KPrint
            ("SF: Failed to read response due to the mismatch of len and response (%zu bytes): %d\n",
             len, ret);
    }

    return ret;
}

int spi_flash_cmd_write(struct spi_flash *slave, UINT8 * u8_cmd, const void *data, int data_len,
                        int sts_hw)
{
    int ret;

    ret = slave->spi_xfer(slave, data_len, u8_cmd, NULL, SPI_XFER_CMD_STATE, sts_hw);
    if (ret) {
        KPrint("SF: Failed to send command %02x: %d\n", u8_cmd[0], ret);
        return ret;
    } else if (data_len != 0) {
        ret =
            slave->spi_xfer(slave, data_len, data, NULL,
                            SPI_XFER_DATA_STATE | SPI_XFER_CHECK_CMD_COMPLETE, sts_hw);
        if (ret) {
            KPrint("SF: Failed to write data (%zu bytes): %d\n", data_len, ret);
        }
    }

    return ret;
}

int spi_flash_cmd_read(struct spi_flash *slave, UINT8 * u8_cmd, void *data, int data_len,
                       int sts_hw)
{
    int ret;

    ret = slave->spi_xfer(slave, data_len, u8_cmd, NULL, SPI_XFER_CMD_STATE, sts_hw);
    if (ret) {
        KPrint("SF: Failed to send command %02x: %d\n", u8_cmd[0], ret);
        return ret;
    } else if (data_len != 0) {
        ret =
            slave->spi_xfer(slave, data_len, NULL, data,
                            SPI_XFER_DATA_STATE | SPI_XFER_CHECK_CMD_COMPLETE, sts_hw);
        if (ret) {
            KPrint("SF: Failed to read data (%zu bytes): %d\n", data_len, ret);
        }
    }

    return ret;
}

/* Wait until BUSY bit clear
 */
void flash_wait_busy_ready(struct spi_flash *flash)
{
    UINT8 rd_sts_cmd[1];
    UINT8 status;

    if (flash_type == FLASH_SPI_NAND) {
        rd_sts_cmd[0] = COMMAND_GET_FEATURES;
        rd_sts_cmd[1] = 0xC0;
    } else {
        rd_sts_cmd[0] = COMMAND_READ_STATUS1;
    }

    do {
        if (spi_flash_cmd(flash, rd_sts_cmd, NULL, 0, 0)) {
            KPrint("Failed to check status!\n");
            break;
        }

        ftspi020_read_status(&status);
    } while (status & FLASH_STS_BUSY);
}

int flash_set_quad_enable(struct spi_flash *flash, int chip_id)
{
    UINT8 rd_sts_cmd[1];
    UINT8 wr_sts_cmd[1];
    UINT8 status[2], tmp, len;
    UINT8 u8_wr_en_cmd[5];

    /* check if the flash is busy */
    flash_wait_busy_ready(flash);

    /* read the status from Status Register-1 */
    rd_sts_cmd[0] = COMMAND_READ_STATUS1;
    if (spi_flash_cmd(flash, rd_sts_cmd, NULL, 0, 0)) {
        KPrint("flash_set_quad_enable fail 1\n");
        return -1;
    }
    ftspi020_read_status(&status[0]);

    if (chip_id == 0xEF) {      //winbond
        /* read the status from Status Register-1 */
        rd_sts_cmd[0] = COMMAND_READ_STATUS2;
        if (spi_flash_cmd(flash, rd_sts_cmd, NULL, 0, 0)) {
            KPrint("flash_set_quad_enable fail 2\n");
            return -1;
        }
        ftspi020_read_status(&status[1]);
        tmp = (1 << 1);
        len = 2;
        status[1] |= tmp;
    } else {
        tmp = (1 << 6);
        len = 1;
        status[0] |= tmp;
    }

    u8_wr_en_cmd[0] = COMMAND_WRITE_ENABLE;
    if (spi_flash_cmd(flash, u8_wr_en_cmd, NULL, 0, 0)) {
        KPrint("flash_set_quad_enable fail 3\n");
        return -1;
    }

    wr_sts_cmd[0] = COMMAND_WRITE_STATUS;
    if (spi_flash_cmd_write(flash, wr_sts_cmd, (const void *)status, len, 1)) {
        KPrint("flash_set_quad_enable fail 4\n");
        return -1;
    }
    return 0;
}

static int spi_xfer(struct spi_flash *flash, unsigned int len, const void *dout, void *din,
                    unsigned long flags, int sts_hw)
{
    struct ftspi020_cmd spi_cmd;
    UINT8 *u8_data_out = (UINT8 *) dout;

    memset(&spi_cmd, 0, sizeof(struct ftspi020_cmd));

    /* send the instruction */
    if (flags & SPI_XFER_CMD_STATE) {
        spi_cmd.start_ce = flash->ce;
        spi_cmd.ins_code = *u8_data_out;
        spi_cmd.intr_en = cmd_complete_intr_enable;

        switch (spi_cmd.ins_code) {
        case CMD_READ_ID:
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.write_en = spi_read;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            spi_cmd.data_cnt = 3;
            if (flash_type == FLASH_SPI_NAND) {
                spi_cmd.spi_addr = (UINT32) * (u8_data_out + 1);
                spi_cmd.addr_len = addr_1byte;
            }
            break;
        case CMD_RESET:
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.write_en = spi_write;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            break;
        case COMMAND_WRITE_ENABLE:
/*	      	
		  case COMMAND_WRITE_DISABLE:
		    if (write_enable == spi_cmd.ins_code)
		        goto exit;
*/
            write_enable = spi_cmd.ins_code;
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.write_en = spi_write;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            break;
        case COMMAND_ERASE_SECTOR:     //erase 4K
            if (flash->mode_3_4 == 4) {
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
            spi_cmd.read_status = sts_hw ? read_status_by_hw : read_status_by_sw;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            break;
        case COMMAND_WRITE_PAGE:       //nsboot not need to use write command
            break;

        case COMMAND_WINBOND_QUAD_WRITE_PAGE:
        case COMMAND_QUAD_WRITE_PAGE:  //nsboot not need to use write command
            break;

        case COMMAND_FAST_READ_QUAD_IO:
            if (flash->mode_3_4 == 4) {
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
            if (flash_type == FLASH_SPI_NAND) {
                spi_cmd.spi_addr = (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
                spi_cmd.addr_len = addr_2byte;
            } else {
                if (flash->mode_3_4 == 4) {
                    spi_cmd.spi_addr =
                        (*(u8_data_out + 4) << 24) | (*(u8_data_out + 3) << 16) |
                        (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
                    spi_cmd.addr_len = addr_4byte;
                } else {
                    spi_cmd.spi_addr =
                        (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) << 8) |
                        (*(u8_data_out + 1));
                    spi_cmd.addr_len = addr_3byte;
                }
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
        case COMMAND_GET_FEATURES:
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.write_en = spi_read;
            spi_cmd.read_status_en = read_status_enable;
            spi_cmd.read_status = sts_hw ? read_status_by_hw : read_status_by_sw;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            spi_cmd.spi_addr = (UINT32) * (u8_data_out + 1);
            spi_cmd.addr_len = addr_1byte;
            break;
        case COMMAND_PAGE_READ:
            if (flash->mode_3_4 == 4) {
                spi_cmd.spi_addr =
                    (*(u8_data_out + 4) << 24) | (*(u8_data_out + 3) << 16) | 
                    (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
                spi_cmd.addr_len = addr_4byte;                
            } else {
                spi_cmd.spi_addr =
                    (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
                spi_cmd.addr_len = addr_3byte;
            }            
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.data_cnt = 0;
            spi_cmd.write_en = spi_write;
            spi_cmd.read_status_en = read_status_disable;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            break;
        default:
            KPrint("spi_xfer: command 0x%x is not supported! \n", spi_cmd.ins_code);
            return -1;
            break;
        }

        /* sent the command out */
        ftspi020_issue_cmd(&spi_cmd);

    } else if (flags & SPI_XFER_DATA_STATE) {
        /* read the data */
        ftspi020_data_access((UINT8 *) dout, (UINT8 *) din, len);
    }

    /* check command complete */
    if (flags & SPI_XFER_CHECK_CMD_COMPLETE)
        ftspi020_wait_cmd_complete();

    return 0;
}

static int spi_dataflash_read_fast(struct spi_flash *flash, read_type_t u8_type, UINT32 u32_offset,
                                   size_t len, void *buf)
{
    int ret, i, loop;
    UINT8 u8_rd_cmd[5] = { 0 };

    //KPrint("spi_dataflash_read_fast offset: 0x%x. size: 0x%x\n", u32_offset, len);
    if (flash_type == FLASH_SPI_NAND) {
        UINT32 start_page;

        start_page = u32_offset / flash->nand_parm.page_size;

        if (len < FLASH_NAND_PAGE_SZ)
            loop = 1;
        else
            loop = len / FLASH_NAND_PAGE_SZ;

        for (i = 0; i < loop; i++) {
            len = FLASH_NAND_PAGE_SZ + FLASH_NAND_SPARE_SZ;
            /* send PAGE READ to cache command first for SPI NAND */
            flash_wait_busy_ready(flash);

            memset(u8_rd_cmd, 0, 5);
            u8_rd_cmd[0] = COMMAND_PAGE_READ;
            /* assign row address */
            u8_rd_cmd[1] = start_page & 0xFF;
            u8_rd_cmd[2] = ((start_page & 0xFF00) >> 8);
            u8_rd_cmd[3] = ((start_page & 0xFF0000) >> 16);

            ret = spi_flash_cmd(flash, u8_rd_cmd, NULL, 0, 0);

            flash_wait_busy_ready(flash);

            u8_rd_cmd[0] = COMMAND_FAST_READ;
            //u8_rd_cmd[0] = COMMAND_FAST_READ_QUAD_IO;

            /* assign column address */
            u8_rd_cmd[1] = 0x00;
            u8_rd_cmd[2] = 0x00;

            ret = spi_flash_cmd_read(flash, u8_rd_cmd, buf, len, 0);

            start_page++;
            buf += FLASH_NAND_PAGE_SZ;
        }
    } else {
        flash_wait_busy_ready(flash);

        u8_rd_cmd[0] = COMMAND_FAST_READ;
        //u8_rd_cmd[0] = COMMAND_FAST_READ_QUAD_IO;

        u8_rd_cmd[1] = u32_offset & 0xFF;
        u8_rd_cmd[2] = ((u32_offset & 0xFF00) >> 8);
        u8_rd_cmd[3] = ((u32_offset & 0xFF0000) >> 16);
        if (flash->mode_3_4 == 4)
            u8_rd_cmd[4] = ((u32_offset & 0xFF000000) >> 24);

        ret = spi_flash_cmd_read(flash, u8_rd_cmd, buf, len, 0);
    }

    return ret;
}

/* u32_offset: byte offset
 * len: how many bytes.
 */
static int spi_dataflash_write_fast(struct spi_flash *flash, write_type_t u8_type,
                                    UINT32 u32_offset, size_t len, void *buf)
{
    return 0;                   //nsboot not need to use write command
}

/* u32_offset: byte offset
 * len: how many bytes.
 */
static int spi_dataflash_erase_fast(struct spi_flash *flash, erase_type_t u8_type,
                                    UINT32 u32_offset, size_t len)
{
    int ret = 0, addr, erase_size;
    UINT8 u8_er_cmd[5];
    UINT8 cmd_code;

    if (u8_type == SECTOR_ERASE) {
        cmd_code = COMMAND_ERASE_SECTOR;
        erase_size = flash->erase_sector_size;
        u32_offset &= ~(erase_size - 1);
    } else if (u8_type == BLOCK_32K_ERASE) {
        cmd_code = COMMAND_ERASE_32K_BLOCK;
        erase_size = 32 << 10;
        u32_offset &= ~((32 << 10) - 1);
    } else {
        cmd_code = COMMAND_ERASE_64K_BLOCK;
        erase_size = 64 << 10;
        u32_offset &= ~((64 << 10) - 1);
    }

    /* check if the flash is busy */
    flash_wait_busy_ready(flash);

    for (addr = u32_offset; addr < (u32_offset + len); addr += erase_size) {
        u8_er_cmd[0] = COMMAND_WRITE_ENABLE;
        ret = spi_flash_cmd(flash, u8_er_cmd, NULL, 0, 0);
        if (ret)
            break;

        u8_er_cmd[0] = cmd_code;
        u8_er_cmd[1] = addr & 0xFF;
        u8_er_cmd[2] = ((addr & 0xFF00) >> 8);
        u8_er_cmd[3] = ((addr & 0xFF0000) >> 16);
        if (flash->mode_3_4 == 4)
            u8_er_cmd[4] = ((addr & 0xFF000000) >> 24);

        ret = spi_flash_cmd(flash, u8_er_cmd, NULL, 0, 0);
        if (ret)
            break;

        /* check if the flash is busy */
        flash_wait_busy_ready(flash);
    }

    return ret;
}

static int spi_dataflash_erase_all(struct spi_flash *flash)
{
    UINT8 er_all_cmd[1];

    er_all_cmd[0] = COMMAND_WRITE_ENABLE;
    if (spi_flash_cmd(flash, er_all_cmd, NULL, 0, 0))
        return -1;

    er_all_cmd[0] = COMMAND_ERASE_CHIP;
    if (spi_flash_cmd(flash, er_all_cmd, NULL, 0, 0))
        return -1;

    /* check if the flash is busy */
    flash_wait_busy_ready(flash);

    return 0;
}

/* enable: 1 for 4 byte mode, 0 for exit 4 byte mode 
 */
int spi_dataflash_en4b(struct spi_flash *flash, int enable)
{
    UINT8 u8_wr_en_cmd[1];
    int ret = 0;

    if (flash_en4b == enable)
        return 0;

    flash_en4b = enable;

    /* check if the flash is busy */
    flash_wait_busy_ready(flash);

    if (enable)
        u8_wr_en_cmd[0] = COMMAND_EN4B;
    else
        u8_wr_en_cmd[0] = COMMAND_EX4B;

    if (spi_flash_cmd(flash, u8_wr_en_cmd, NULL, 0, 1)) {
        ret = -1;
    }

    return ret;
}

/* setup the flash information, such callback function.
 * here we assume all flashes support these common features.
 */
void ftspi020_setup_flash(struct spi_flash *flash)
{
    memset(flash, 0, sizeof(struct spi_flash));

    flash->ce = start_ce0;      // temporary
    if (flash_type == FLASH_SPI_NAND) {
        flash->nand_parm.block_pg = FLASH_NAND_BLOCK_PG;        // temporary
        flash->nand_parm.page_size = FLASH_NAND_PAGE_SZ;        // temporary
        flash->nand_parm.spare_size = FLASH_NAND_SPARE_SZ;      // temporary
        flash->nand_parm.block_size = FLASH_NAND_BLOCK_PG * FLASH_NAND_PAGE_SZ; // temporary
    } else {
        flash->erase_sector_size = FLASH_SECTOR_SZ;
        flash->page_size = FLASH_PAGE_SZ;
        flash->nr_pages = flash->size / flash->page_size;
    }
    flash->spi_xfer = spi_xfer;
    flash->read = spi_dataflash_read_fast;
    flash->write = spi_dataflash_write_fast;
    flash->erase = spi_dataflash_erase_fast;
    flash->erase_all = spi_dataflash_erase_all;
    //flash->report_status = spi_dataflash_report_status;
    //flash->en4b = spi_dataflash_en4b;
    if (flash_type == FLASH_SPI_NAND)
        return;

    if (read_reg(REG_PMU_BASE + 0x4) & (1 << 23))
        flash->mode_3_4 = 4;
    else
        flash->mode_3_4 = 3;

    KPrint("SPI jump setting is %d bytes mode\n", flash->mode_3_4);
}
