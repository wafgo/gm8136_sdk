#include <common.h>
#include "spi020_nand_flash.h"
#include "spi_nand_flash_internal.h"
#include <malloc.h>
#include <config.h>

#if defined(CONFIG_CMD_NAND)
#if !defined(CFG_NAND_LEGACY)
#include <nand.h>

typedef struct bi_table {
    /* This array size is related to USB_BUFF_SZ defined in usb_scsi.h */
    unsigned int bi_status[256];        //each bit indicates a block. 1 for good, 0 for bad, it can allocate 8Gb flash
} BI_TABLE;

BI_TABLE bi_table;

#ifdef CONFIG_SPI_QUAD
static int quad_enable = -1;
#endif
//static int write_enable = -1;

struct spi020_flash spi_flash_info = { 0 };

#define IDCODE_LEN 4    // ID has n bytes
NOR_SYSHDR *spi_sys_hdr;
unsigned char spi_pgbuf[FLASH_NAND_PAGE_SZ + FLASH_NAND_SPARE_SZ];
unsigned char header_pgbuf[FLASH_NAND_PAGE_SZ + FLASH_NAND_SPARE_SZ];
int command_mode = 0, oob_block_num = 0, cmdfunc_status = NAND_STATUS_READY;
int read_id_time = 0, ID_point = 0;
int prog0_page_addr = 0;
unsigned char buf[IDCODE_LEN] = { 0 };

SPI_NAND_SYSHDR *nand_sys_hdr;

extern void spi_nand_init(void);
// ************************* flash releated functions *************************
void transfer_param(SPI_NAND_SYSHDR * header)
{
    spi_flash_info.ce = 0;
    //printf("SF%d,name=%s\n", params->l2_page_size,params->name);
    /* winbond read status command is dirrerence with other chip */
#if 0
    if (flash->name[0] == 'W') {
        spi_flash_info.chip_type = 1;
        printf("winbond chip\n");
    } else
        spi_flash_info.chip_type = 0;
#endif
    SPI_DEBUG("header = 0x%x, 0x%x, 0x%x, 0x%x\n", header->bootm_size,
              header->nandfixup.nand_pagesz, header->nandfixup.nand_sparesz_inpage,
              header->nandfixup.nand_numpgs_blk);

    spi_flash_info.nand_parm.page_size = header->nandfixup.nand_pagesz;
    spi_flash_info.nand_parm.spare_size = header->nandfixup.nand_sparesz_inpage;
    spi_flash_info.nand_parm.block_pg = header->nandfixup.nand_numpgs_blk;
}

int spi020_flash_cmd(struct spi020_flash *spi, UINT8 * u8_cmd, void *response, size_t len)
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

int spi020_flash_cmd_write(struct spi020_flash *spi, UINT8 * u8_cmd, const void *data, int data_len)
{
    int ret;

    ret = spi_xfer(spi, data_len, u8_cmd, NULL, SPI_XFER_CMD_STATE);
    if (ret) {
        printf("SF: Failed to send command %02x: %d\n", u8_cmd[0], ret);
        return ret;
    } else if (data_len != 0) {
        ret = spi_xfer(spi, data_len, data, NULL, SPI_XFER_DATA_OUT_STATE);     //SPI_XFER_CHECK_CMD_COMPLETE);
        if (ret) {
            printf("SF: Failed to write data (%zu bytes): %d\n", data_len, ret);
        }
    }

    return ret;
}

int spi020_flash_cmd_read(struct spi020_flash *spi, UINT8 * u8_cmd, void *data, int data_len)
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
int flash_wait_busy_ready(struct spi020_flash *spi)
{
    UINT8 rd_sts_cmd[1];
    UINT8 status = 0;

    rd_sts_cmd[0] = COMMAND_GET_FEATURES;
    rd_sts_cmd[1] = 0xC0;

    do {
        if (spi020_flash_cmd(spi, rd_sts_cmd, NULL, 0)) {
            printf("Failed to check status!\n");
            break;
        }

        ftspi020_read_status(&status);
    } while (status & FLASH_STS_BUSY);

    SPI_DEBUG("get feature = 0x%x\n", status);
    return status;
}

#ifdef CONFIG_SPI_QUAD
static int flash_set_quad_enable(struct spi020_flash *spi, int en)
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

int spi_xfer(struct spi020_flash *spi, unsigned int len, const void *dout, void *din,
             unsigned long flags)
{
    struct ftspi020_cmd spi_cmd;
    UINT8 *u8_data_out = (UINT8 *) dout;

    memset(&spi_cmd, 0, sizeof(struct ftspi020_cmd));

    /* send the instruction */
    if (flags & SPI_XFER_CMD_STATE) {

        spi_cmd.ins_code = *u8_data_out;
        //printf("dout=0x%x,din=0x%x,%d,%d\n",*(UINT8 *)dout,*(UINT8 *)din, len, spi->cs);
        spi_cmd.intr_en = cmd_complete_intr_enable;
#ifdef CONFIG_CMD_FPGA
        spi_cmd.start_ce = 0;
#endif
        SPI_DEBUG("spi cmd=0x%x\n", spi_cmd.ins_code);
        switch (spi_cmd.ins_code) {
        case COMMAND_READ_ID:
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.write_en = spi_read;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            spi_cmd.data_cnt = IDCODE_LEN;
            spi_cmd.spi_addr = (UINT32) * (u8_data_out + 1);
            spi_cmd.addr_len = addr_1byte;
            break;
        case COMMAND_RESET:
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.write_en = spi_write;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            break;
        case COMMAND_WRITE_ENABLE:
        case COMMAND_WRITE_DISABLE:
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
        case COMMAND_ERASE_128K_BLOCK:
            spi_cmd.spi_addr =
                (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
            spi_cmd.addr_len = addr_3byte;

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
        case COMMAND_WRITE_PAGE:       //cmd as the same with COMMAND_PROGRAM_EXECUTE
            spi_cmd.spi_addr = (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
            spi_cmd.addr_len = addr_2byte;

            spi_cmd.ins_len = instr_1byte;
            spi_cmd.data_cnt = len;
            spi_cmd.write_en = spi_write;
            spi_cmd.read_status_en = read_status_disable;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            break;
        case COMMAND_WINBOND_QUAD_WRITE_PAGE:
        case COMMAND_QUAD_WRITE_PAGE:
            spi_cmd.spi_addr =
                (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
            spi_cmd.addr_len = addr_3byte;

            spi_cmd.ins_len = instr_1byte;
            spi_cmd.data_cnt = len;
            spi_cmd.write_en = spi_write;
            spi_cmd.read_status_en = read_status_disable;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_quad_mode;
            break;
        case COMMAND_FAST_READ_QUAD_IO:
            spi_cmd.spi_addr =
                (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
            spi_cmd.addr_len = addr_3byte;

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
        case COMMAND_READ_DATA:
        case COMMAND_FAST_READ:
            spi_cmd.spi_addr = (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
            spi_cmd.addr_len = addr_2byte;

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
            spi_cmd.spi_addr = (UINT32) * (u8_data_out + 1);
            spi_cmd.addr_len = addr_1byte;
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.write_en = spi_read;
            spi_cmd.read_status_en = read_status_enable;
            spi_cmd.read_status = read_status_by_sw;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            break;
        case COMMAND_PAGE_READ:
            spi_cmd.spi_addr =
                (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
            spi_cmd.addr_len = addr_3byte;
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.data_cnt = 0;
            spi_cmd.write_en = spi_write;
            spi_cmd.read_status_en = read_status_disable;
            spi_cmd.dtr_mode = dtr_disable;
            spi_cmd.spi_mode = spi_operate_serial_mode;
            break;
        case COMMAND_PROGRAM_EXECUTE:
            spi_cmd.spi_addr =
                (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
            spi_cmd.addr_len = addr_3byte;
            spi_cmd.ins_len = instr_1byte;
            spi_cmd.data_cnt = len;
            spi_cmd.write_en = spi_write;
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
        SPI_DEBUG("read len = %d, buf = 0x%x\n", len, din);
        ftspi020_data_access(1, (UINT8 *) din, len);
        SPI_DEBUG("read data finish\n");
    }
#if 0
    else if (flags & SPI_XFER_DATA_OUT_STATE) {
        /* send the data */
        printf("write len = %d, buf = 0x%x\n", len, dout);
        ftspi020_data_access(0, (UINT8 *) dout, len);
        printf("write data finish\n");
    }
#endif

    /* check command complete */
    if (flags & SPI_XFER_CHECK_CMD_COMPLETE)
        ftspi020_wait_cmd_complete();

xfer_exit:
    return 0;
}

int read_data(struct spi020_flash *flash, size_t len, void *buf)
{
    int ret;
    u8 u8_rd_cmd[5];
        
    memset(u8_rd_cmd, 0, 5);

#ifdef CONFIG_SPI_QUAD
    flash_set_quad_enable(flash, 1);
    u8_rd_cmd[0] = COMMAND_FAST_READ_QUAD_IO;
#else
    u8_rd_cmd[0] = COMMAND_FAST_READ;
#endif
    if (len == spi_flash_info.nand_parm.spare_size) {
        u8_rd_cmd[1] = spi_flash_info.nand_parm.page_size & 0xFF;
        u8_rd_cmd[2] = ((spi_flash_info.nand_parm.page_size & 0xFF00) >> 8);        
    } else {
        u8_rd_cmd[1] = 0x00;
        u8_rd_cmd[2] = 0x00;
    }
    ret = spi020_flash_cmd_read(flash, u8_rd_cmd, buf, len);

    return ret;    
}

int spi_flash_cmd_read_fast(struct spi020_flash *flash, u32 offset, size_t len, void *buf)
{
    int ret = 0;
    u8 u8_rd_cmd[5];

    /* send PAGE READ command first for SPI NAND */
    //flash_wait_busy_ready(flash);

    memset(u8_rd_cmd, 0, 5);
    u8_rd_cmd[0] = COMMAND_PAGE_READ;
    /* assign row address */
    u8_rd_cmd[1] = offset & 0xFF;
    u8_rd_cmd[2] = ((offset & 0xFF00) >> 8);
    u8_rd_cmd[3] = ((offset & 0xFF0000) >> 16);

    ret = spi_flash_cmd(flash, u8_rd_cmd, NULL, 0);

    /* check if the flash is busy */
    if (flash_wait_busy_ready(flash) & FLASH_NAND_R_FAIL) {
        printf("read offset = 0x%x ECC fail\n", offset * spi_flash_info.nand_parm.page_size);
        ret = 1;
    }

    return ret;
}

/* offset: byte offset
 * len: how many bytes.
 */
int spi_flash_cmd_write_prog0(struct spi020_flash *flash, size_t len, const void *buf)
{
    int ret;
    UINT8 u8_wr_en_cmd[1], u8_wr_cmd[5];
    UINT8 *u8_buf = (UINT8 *) buf;

    /* check if the flash is busy */
    //flash_wait_busy_ready(flash);

#ifdef CONFIG_SPI_QUAD
    ret = flash_set_quad_enable(flash, 1);
    if (ret) {
        ret = -1;
        goto prog0_exit;
    }
#endif

    u8_wr_en_cmd[0] = COMMAND_WRITE_ENABLE;
    if (spi020_flash_cmd(flash, u8_wr_en_cmd, NULL, 0)) {
        printf("COMMAND_WRITE_ENABLE fail\n");
        ret = -1;
        goto prog0_exit;
    }

    /* send PROGRAM LOAD command for SPI NAND */
    u8_wr_cmd[0] = COMMAND_PROGRAM_LOAD;
    /* assign column address */
    u8_wr_cmd[1] = 0x00;
    u8_wr_cmd[2] = 0x00;

    //ret = spi_flash_cmd_write(flash, u8_wr_cmd, u8_buf, len, 0);
    ret = spi020_flash_cmd_write(flash, u8_wr_cmd, u8_buf, len);
    if (ret) {
        printf("COMMAND_WRITE_ENABLE fail\n");
        ret = -1;
        goto prog0_exit;
    }

prog0_exit:
    return ret;
}

int spi_flash_cmd_write_prog1(struct spi020_flash *flash, u32 offset)
{
    int ret = 0;
    UINT8 u8_wr_cmd[5];

    ftspi020_wait_cmd_complete();
    //flash_wait_busy_ready(flash);

#ifdef CONFIG_SPI_QUAD
    //only for QUAD_PROGRAM
    if (spi_flash_info.chip_type)
        u8_wr_cmd[0] = COMMAND_WINBOND_QUAD_WRITE_PAGE;
    else
        u8_wr_cmd[0] = COMMAND_QUAD_WRITE_PAGE;
#else
    u8_wr_cmd[0] = COMMAND_PROGRAM_EXECUTE;
#endif

    /* assign row address */
    u8_wr_cmd[1] = offset & 0xFF;
    u8_wr_cmd[2] = ((offset & 0xFF00) >> 8);
    u8_wr_cmd[3] = ((offset & 0xFF0000) >> 16);

    if (spi020_flash_cmd(flash, u8_wr_cmd, NULL, 0)) {
        printf("COMMAND_PROGRAM_EXECUTE fail\n");
        ret = -1;
    }

    /*
       if (spi_flash_cmd(flash, u8_wr_cmd, NULL, 0))
       ret = -1;
       else
       ret = 0; */

    if (flash_wait_busy_ready(flash) & FLASH_NAND_P_FAIL) {
        printf("program result fail\n");
        ret = 1;
    }        

    return ret;
}

/* offset: byte offset
 * len: how many bytes.
 */
int spi_flash_cmd_erase(struct spi020_flash *flash, u32 offset)
{
    int ret = 0;
    UINT8 u8_er_cmd[5];

    /* check if the flash is busy */
    //flash_wait_busy_ready(flash);

    u8_er_cmd[0] = COMMAND_WRITE_ENABLE;
    ret = spi020_flash_cmd(flash, u8_er_cmd, NULL, 0);
    if (ret) {
        printf("cmd_erase write enable fail\n");
        return ret;
    }

    u8_er_cmd[0] = COMMAND_ERASE_128K_BLOCK;
    u8_er_cmd[1] = offset & 0xFF;
    u8_er_cmd[2] = ((offset & 0xFF00) >> 8);
    u8_er_cmd[3] = ((offset & 0xFF0000) >> 16);

    ret = spi020_flash_cmd(flash, u8_er_cmd, NULL, 0);
    if (ret) {
        printf("cmd_erase erase fail\n");
        return ret;
    }

    /* check if the flash is busy */
    if (flash_wait_busy_ready(flash) & FLASH_NAND_E_FAIL) {
        printf("erase result fail\n");
        ret = 1;
    }

    return ret;
}

static int spi_flash_read_write(struct spi020_flash *spi,
                                const u8 * cmd, size_t cmd_len,
                                const u8 * data_out, u8 * data_in, size_t data_len)
{
    unsigned long flags;
    int ret;

    if (data_len == 0)
        flags = (SPI_XFER_CMD_STATE | SPI_XFER_CHECK_CMD_COMPLETE);     //(SPI_XFER_END | SPI_XFER_CHECK_CMD_COMPLETE);

    ret = spi_xfer(spi, cmd_len * 8, cmd, NULL, flags);
    if (ret) {
        printf("SF: Failed to send command (%zu bytes): %d\n", cmd_len, ret);
    } else if (data_len != 0) {
        if (data_in == NULL) {  // write
            if (data_len % 4) {
                if (*cmd != CMD_WRITE_STATUS)
                    printf("data len %x not 4 times\n", data_len);
                return ret;
            }
        }
        ret = spi_xfer(spi, data_len * 8, data_out, data_in, SPI_XFER_CHECK_CMD_COMPLETE);      //SPI_XFER_END, 0);
        if (ret)
            printf("SF: Failed to transfer %zu bytes of data: %d\n", data_len, ret);
    }

    return ret;
}

int spi_flash_cmd(struct spi020_flash *spi, u8 * cmd, void *response, size_t len)
{
    return spi_flash_cmd_read(spi, cmd, 1, response, len);
}

int spi_flash_cmd_read(struct spi020_flash *spi, const u8 * cmd,
                       size_t cmd_len, void *data, size_t data_len)
{
    return spi_flash_read_write(spi, cmd, cmd_len, NULL, data, data_len);
}

int spi_flash_cmd_write(struct spi020_flash *spi, const u8 * cmd, size_t cmd_len,
                        const void *data, size_t data_len)
{
    return spi_flash_read_write(spi, cmd, cmd_len, data, NULL, data_len);
}

int spi_flash_cmd_write_status(struct spi020_flash *flash, u8 sr)
{
    u8 cmd;
    int ret;

    ret = spi_flash_cmd_write_enable(flash);
    if (ret < 0) {
        debug("SF: enabling write failed\n");
        return ret;
    }

    cmd = CMD_WRITE_STATUS;
    ret = spi_flash_cmd_write(flash, &cmd, 1, &sr, 1);
    if (ret) {
        debug("SF: fail to write status register\n");
        return ret;
    }

    return 0;
}

static uint8_t scan_ff_pattern[] = { 0xff, 0xff };

static struct nand_bbt_descr bbt_descr = {
    .options = 0,
    .offs = 0,
    .len = 1,
    .pattern = scan_ff_pattern
};

static struct nand_ecclayout oob_descr_2048 = { //???
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

static int nand_verify_buf32(struct mtd_info *mtd, const u_char * buf, int len)
{
    printf("%s\n", __FUNCTION__);
    printf("buf=0x%x,len=%d\n", (unsigned int)buf, (unsigned int)len);

    return 0;
}

static void nandc023_cmd_ctrl(struct mtd_info *mtd, int dat, unsigned int ctrl)
{
    NANDC_DEBUG3("%s: WARNING, this function does not implement with the NANDC023!\n",
                 __FUNCTION__);
}

static int nandc023_wait(struct mtd_info *mtd, struct nand_chip *this)
{
    NANDC_DEBUG1("%s\n", __FUNCTION__);
    return cmdfunc_status;
}

int Check_command_status(UINT8 u8_channel, UINT8 detect)
{
    return 1;
}

int nand_read_buffer(u_char * buf, int len)
{
    int value;

    if (len == FLASH_NAND_SPARE_SZ) {
        //memcpy(buf, (spi_pgbuf + FLASH_NAND_PAGE_SZ), FLASH_NAND_SPARE_SZ);
        read_data(&spi_flash_info, len, buf);
#if 1 //debug        
        if (*buf == 0xFF)
#endif
        {
            value = bi_table.bi_status[oob_block_num / 32];
            value |= (1 << (oob_block_num % 32));
            bi_table.bi_status[oob_block_num / 32] = value;
        }
        //else
        //    printf("<B = 0x%x, addr = 0x%x>", *buf, oob_block_num * FLASH_NAND_PAGE_SZ * spi_flash_info.nand_parm.block_pg);
    }

    if (len == FLASH_NAND_PAGE_SZ) {
        SPI_DEBUG("nand_read_buffer data len = %d, buf addr = 0x%x\n", len, buf);
        read_data(&spi_flash_info, len, buf);
        //memcpy(buf, spi_pgbuf, FLASH_NAND_PAGE_SZ);
    }

    if (command_mode == NAND_CMD_READID) {
        SPI_DEBUG("get ID len 4\n");
        ftspi020_data_access(1, buf, IDCODE_LEN);
    }

    return 1;
}

static u_char nandc023_read_byte(struct mtd_info *mtd)
{
    NANDC_DEBUG1("%s in\n", __FUNCTION__);
    if (command_mode == NAND_CMD_READID) {
        if (read_id_time == 0) {
            nand_read_buffer(buf, IDCODE_LEN);
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

static u16 nandc023_read_word(struct mtd_info *mtd)
{
    printf("%s: UNIMPLEMENTED.\n", __FUNCTION__);
    return 0;
}

static void nandc023_read_buf(struct mtd_info *mtd, u_char * const buf, int len)
{
    NANDC_DEBUG3("%s in\n", __FUNCTION__);

    NANDC_DEBUG1("len = %d\n", len);
    nand_read_buffer(buf, len);

    NANDC_DEBUG3("%s exit\n", __FUNCTION__);
    return;
}

static void nandc023_write_buf(struct mtd_info *mtd, const u_char * buf, int len)
{
    SPI_DEBUG("%s in, len = %d, buf addr = 0x%x\n", __FUNCTION__, len, buf);

    if (len == spi_flash_info.nand_parm.page_size) {
        memcpy(spi_pgbuf, buf, len);
    } else if (len == spi_flash_info.nand_parm.spare_size) {
        SPI_DEBUG("write len = %d, buf = 0x%x\n",
                  spi_flash_info.nand_parm.page_size + spi_flash_info.nand_parm.spare_size,
                  spi_pgbuf);

        if (command_mode == NAND_CMD_BI_TABLE) {
            memset(spi_pgbuf + spi_flash_info.nand_parm.page_size, 0x0, len);
            spi_flash_cmd_write_prog0(&spi_flash_info,
                                      spi_flash_info.nand_parm.page_size +
                                      spi_flash_info.nand_parm.spare_size, spi_pgbuf);
            ftspi020_data_access(0, spi_pgbuf,
                                 spi_flash_info.nand_parm.page_size +
                                 spi_flash_info.nand_parm.spare_size);
            spi_flash_cmd_write_prog1(&spi_flash_info, prog0_page_addr);
        } else {
            memcpy(spi_pgbuf + spi_flash_info.nand_parm.page_size, buf, len);
            ftspi020_data_access(0, spi_pgbuf,
                                 spi_flash_info.nand_parm.page_size +
                                 spi_flash_info.nand_parm.spare_size);
        }

        SPI_DEBUG("write data finish\n");
    } else
        printf("len = %d has some issue\n", len);

    //printf("%s exit\n", __FUNCTION__);
    return;
}

/* 0 is good, 1 is bad */
int ftnandc_read_bbt(int block_num)
{
    int data, ret;

    data = bi_table.bi_status[block_num / 32];
    data = (data >> (block_num % 32)) & 0x1;
    if (data)
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

static void nandc023_cmdfunc(struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
    SPI_DEBUG("nandc023_cmdfunc=0x%x\n", command);

    switch (command) {
    case NAND_CMD_READ0:
        NANDC_DEBUG1
            ("nandc023_cmdfunc: NAND_CMD_READ0, page_addr: 0x%x, size: 0x%x.\n",
             page_addr, spi_flash_info.nand_parm.page_size);

        command_mode = NAND_CMD_READ0;
        spi_flash_cmd_read_fast(&spi_flash_info, page_addr,
                                spi_flash_info.nand_parm.page_size, spi_pgbuf);
        //nand023_read_page(page_addr);
        goto end;

    case NAND_CMD_READ1:
        NANDC_DEBUG1("nandc023_cmdfunc: NAND_CMD_READ1 unimplemented!\n");
        goto end;

    case NAND_CMD_READOOB:
        NANDC_DEBUG1
            ("<nandc023_cmdfunc: NAND_CMD_READOOB, page_addr: 0x%x, size: 0x%x.>",
             page_addr, spi_flash_info.nand_parm.spare_size);

        command_mode = NAND_CMD_READOOB;
        oob_block_num = page_addr / spi_flash_info.nand_parm.block_pg;
        spi_flash_cmd_read_fast(&spi_flash_info, page_addr,
                                spi_flash_info.nand_parm.spare_size, spi_pgbuf);

        // calc block number, mtd->erasesize / mtd->writesize =  one block n pages
        //block_num = page_addr / (mtd->erasesize / mtd->writesize);
        //BI_data = ftnandc_read_bbt(block_num);

        goto end;

    case NAND_CMD_READID:
        NANDC_DEBUG1("nandc023_cmdfunc: NAND_CMD_READID.\n");
        command_mode = NAND_CMD_READID;
        {
            ftspi020_cmd_t spi_cmd;
            //printf("A3\n");
            ftspi020_setup_cmdq(&spi_cmd, COMMAND_READ_ID, instr_1byte, spi_read,
                                spi_operate_serial_mode, IDCODE_LEN);
            ftspi020_wait_cmd_complete();
        }
        goto write_cmd;

    case NAND_CMD_ERASE1:
        NANDC_DEBUG1("nandc023_cmdfunc: NAND_CMD_ERASE1, page_addr: 0x%x.\n", page_addr);
        if(spi_flash_cmd_erase(&spi_flash_info, page_addr))
            cmdfunc_status = NAND_STATUS_FAIL;
        else
            cmdfunc_status = NAND_STATUS_READY;
        //ftnandc023_erase_block(page_addr / flash_parm.u16_page_in_block, 1, &flash_parm);
        goto end;

    case NAND_CMD_ERASE2:
        NANDC_DEBUG1("nandc023_cmdfunc: NAND_CMD_ERASE2 not need to implement.\n");
        goto end;

    case NAND_CMD_PAGEPROG:
        /* sent as a multicommand in NAND_CMD_SEQIN */
        page_addr = prog0_page_addr;
        SPI_DEBUG("nandc023_cmdfunc: NAND_CMD_PAGEPROG,  page_addr: 0x%x\n", page_addr);
        if(spi_flash_cmd_write_prog1(&spi_flash_info, page_addr))
            cmdfunc_status = NAND_STATUS_FAIL;
        else
            cmdfunc_status = NAND_STATUS_READY;

        goto end;

    case NAND_CMD_SEQIN:
        /* send PAGE_PROG command(0x1080) */

        SPI_DEBUG("nandc023_cmdfunc: NAND_CMD_SEQIN,  page_addr: 0x%x, column: 0x%x.\n",
                  page_addr, (column >> 1));

        prog0_page_addr = page_addr;
        command_mode = NAND_CMD_SEQIN;
        spi_flash_cmd_write_prog0(&spi_flash_info,
                                  spi_flash_info.nand_parm.page_size +
                                  spi_flash_info.nand_parm.spare_size, spi_pgbuf);
        goto end;

    case NAND_CMD_BI_TABLE:
        /* send PAGE_PROG command(0x1080) */
        printf
            ("markbad, page_addr: 0x%x, column: 0x%x.\n",
             page_addr, (column >> 1));

        prog0_page_addr = page_addr;
        command_mode = NAND_CMD_BI_TABLE;

        goto end;

    case NAND_CMD_STATUS:
        NANDC_DEBUG1("nandc023_cmdfunc: NAND_CMD_STATUS.\n");
        goto end;

    case NAND_CMD_RESET:
        NANDC_DEBUG1("nandc023_cmdfunc: NAND_CMD_RESET.\n");
        {
            ftspi020_cmd_t spi_cmd;

            FTSPI020_8BIT(INTR_CONTROL_REG) = 0x0;      //disable DMA enable
            printf("reset flash\n");
            memset(&spi_cmd, 0, sizeof(ftspi020_cmd_t));
            ftspi020_setup_cmdq(&spi_cmd, COMMAND_RESET, instr_1byte, spi_write,
                                spi_operate_serial_mode, 0);
            ftspi020_wait_cmd_complete();

            printf("disable flash write protect\n");
            memset(&spi_cmd, 0, sizeof(ftspi020_cmd_t));
            ftspi020_setup_cmdq(&spi_cmd, COMMAND_SET_FEATURES, instr_1byte, spi_write,
                                spi_operate_serial_mode, 1);
            FTSPI020_32BIT(SPI020_DATA_PORT) = 0;
            ftspi020_wait_cmd_complete();

#ifdef CONFIG_SPI_DMA
            FTSPI020_8BIT(INTR_CONTROL_REG) = 0x1;
#else
            FTSPI020_8BIT(INTR_CONTROL_REG) = 0x0;
#endif
        }
        goto end;

    default:
        printf("nandc023_cmdfunc: error, unsupported command (0x%x).\n", command);
        goto end;
    }

write_cmd:

    /*  wait_event: */
    if (!Check_command_status(SPI020_CHANNEL, 0)) {
        printf("Check_command_status fail");
        return;
    }

end:
    return;
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

static void hwctl(struct mtd_info *mtd, int mode)
{
    //hw do it, not need implement
    NANDC_DEBUG3("%s in\n", __FUNCTION__);
}

int board_nand_init(struct nand_chip *nand)
{
    //struct spi_slave *spi;
    struct spi020_flash flash = { 0 };
    u8 idcode[IDCODE_LEN];

    spi_nand_init();

    SPI_DEBUG("SPI NAND init finished!");

    /* Read the ID codes */

    ftspi020_cmd_t spi_cmd;
    //printf("A3\n");
    ftspi020_setup_cmdq(&spi_cmd, COMMAND_READ_ID, instr_1byte, spi_read, spi_operate_serial_mode,
                        IDCODE_LEN);
    ftspi020_wait_cmd_complete();
    ftspi020_data_access(1, idcode, sizeof(idcode));

    memset(&bi_table, 0, sizeof(BI_TABLE));
#ifdef DEBUG
    printf("SF: Got idcodes\n");
    print_buffer(0, idcode, 1, IDCODE_LEN, 0);
#endif

    spi_flash_cmd_read_fast(&flash, 0, FLASH_NAND_PAGE_SZ + FLASH_NAND_SPARE_SZ, spi_pgbuf);
    read_data(&flash, FLASH_NAND_PAGE_SZ + FLASH_NAND_SPARE_SZ, spi_pgbuf);

    memcpy(header_pgbuf, spi_pgbuf, sizeof(SPI_NAND_SYSHDR));

    nand_sys_hdr = (SPI_NAND_SYSHDR *) header_pgbuf;

    SPI_DEBUG("spi_pgbuf = 0x%x\n", header_pgbuf);
    SPI_DEBUG("spi_pgbuf = 0x%x\n", spi_pgbuf);

    transfer_param(nand_sys_hdr);

    /* check signature */
    if (memcmp((char *)spi_pgbuf, SS_SIGNATURE, 0x6) != 0) {
        printf("NAND: error system header! = 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", spi_pgbuf[0],
               spi_pgbuf[1], spi_pgbuf[2], spi_pgbuf[3], spi_pgbuf[4], spi_pgbuf[5]);

        spi_flash_info.nand_parm.page_size = FLASH_NAND_PAGE_SZ;
        spi_flash_info.nand_parm.spare_size = FLASH_NAND_SPARE_SZ;
        spi_flash_info.nand_parm.block_pg = FLASH_NAND_BLOCK_PG;
        //return NULL;
    }
    // check 0xAA55_Signature
    if ((((INT8U *) spi_pgbuf)[0x1FE] != 0x55) || (((INT8U *) spi_pgbuf)[0x1FF] != 0xAA)) {
        printf("NAND: System Header 0x55AA check fail!\n");
        //return NULL;
    }

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

    nand->ecc.size = 512;
    nand->ecc.bytes = 8;        //???
    nand->ecc.layout = &oob_descr_2048;

    NANDC_DEBUG1("ECC = %d, %d\n", nand->ecc.size, nand->ecc.bytes);

    nand->badblock_pattern = &bbt_descr;        //at start, u-boot try to scan all block with this pattern
    nand->verify_buf = nand_verify_buf32;
    nand->cmd_ctrl = nandc023_cmd_ctrl;

    return 0;
}
#else
#error "U-Boot legacy NAND support not available for SPI NAND."
#endif
#endif
