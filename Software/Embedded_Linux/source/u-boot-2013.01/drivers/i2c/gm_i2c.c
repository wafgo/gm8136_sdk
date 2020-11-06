/*
 * i2c driver
 */

#include <common.h>

#if defined(CONFIG_HARD_I2C)

/* I2C Register */
#define I2CR               	0x0
#define I2SR                0x4
#define I2C_ClockDiv        0x8
#define I2DR                0x0C
#define I2C_SlaveAddr     	0x10
#define I2C_TGSR           	0x14
#define I2C_BMR            	0x18

/* I2C Control Register */
#define I2C_ALIEN                   0x2000  /* Arbitration lose */
#define I2C_SAMIEN                  0x1000  /* slave address match */
#define I2C_STOPIEN                 0x800   /* stop condition */
#define I2C_BERRIEN                 0x400   /* non ACK response */
#define I2C_DRIEN                   0x200   /* data receive */
#define I2C_DTIEN                   0x100   /* data transmit */
#define I2C_TBEN                    0x80    /* transfer byte enable */
#define I2C_ACKNAK                  0x40    /* ack sent */
#define I2C_STOP                    0x20    /* stop */
#define I2C_START                   0x10    /* start */
#define I2C_GCEN                    0x8     /* general call */
#define I2C_SCLEN                   0x4     /* enable clock */
#define I2C_I2CEN                   0x2     /* enable I2C */
#define I2C_I2CRST                  0x1     /* reset I2C */
#define I2C_ENABLE                  (I2C_ALIEN|I2C_SAMIEN|I2C_STOPIEN|I2C_BERRIEN|I2C_DRIEN|I2C_DTIEN|I2C_SCLEN|I2C_I2CEN)

/* I2C Status Register */
#define I2C_CLRAL                   0x400
#define I2C_CLRGC                   0x200
#define I2C_CLRSAM                  0x100
#define I2C_CLRSTOP                 0x80
#define I2C_CLRBERR                 0x40
#define I2C_DR                      0x20
#define I2C_DT                      0x10
#define I2C_BB                      0x8
#define I2C_BUSY                    0x4
#define I2C_ACK                     0x2
#define I2C_RW                      0x1

#define inw(port)			(*(volatile uint *)(port))
#define outw(port, data)	(*(volatile uint *)(port) = data)

//#define DEBUG
#ifdef DEBUG
#define DPRINTF(args...)  printf(args)
#else
#define DPRINTF(args...)
#endif

#define TIMEOUT 100 // Timeout value for reset, tx/rx wait

extern uint u32PMU_ReadPCLK(void);

void i2c_init(int speed, int unused)
{
	u32 freq = u32PMU_ReadPCLK();
	int i;

#if defined(CONFIG_PLATFORM_GM8136) || defined(CONFIG_PLATFORM_GM8139)
    /* Disable i2c0 gating clock off */
    *(volatile u32 *)(CONFIG_PMU_BASE + 0xB8) &= ~(1 << 7);
    *(volatile u32 *)(CONFIG_PMU_BASE + 0x40) |= (1 << 12);
    /* Enable Multi-function Port for I2C Pin-mux  */
    *(volatile u32 *)(CONFIG_PMU_BASE + 0x54) &= 0xFC3FFFFF;
    *(volatile u32 *)(CONFIG_PMU_BASE + 0x54) |= 0x1400000;
#else
    /* Disable i2c0 gating clock off */
	*(volatile u32 *)(CONFIG_PMU_BASE + 0xB8) &= ~(1 << 14);
	*(volatile u32 *)(CONFIG_PMU_BASE + 0x44) |= (1 << 12);
#endif
	udelay(5);

    /* Reset I2C Controller */
    *(volatile u32 *)(CONFIG_I2C_BASE + I2CR) = I2C_I2CRST;
    for(i = 0; i < TIMEOUT; i++) { // Wait for I2C_RST pin to be automatically cleared
        if(!(*(volatile u32 *)(CONFIG_I2C_BASE + I2CR) & I2C_I2CRST))
            break;
    }

	*(volatile u32 *)(CONFIG_I2C_BASE + I2C_TGSR) = (I2C_GSR_Value << 10) | I2C_TSR_Value;
	DPRINTF("APB freq = %d\n",freq);

	i = (((freq / speed) - I2C_GSR_Value) / 2) - 2;
	if(i > 0x3ffff) {
		printf("can't div \n");
        return -1;
    }

	*(volatile u32 *)(CONFIG_I2C_BASE + I2C_ClockDiv) = i;
	DPRINTF("%s: speed: %d, div = %d\n",__FUNCTION__, speed, i);
    return 0;
}

static int tx_wait_busy(void)
{
    int timeout = TIMEOUT;

	for (; timeout > 0; -- timeout)
	if (inw(CONFIG_I2C_BASE + I2SR) & I2C_DT)
		break;
	else
		udelay(1);

	return timeout;
}

static int tx_byte(u8 byte)
{
    int timeout = TIMEOUT;

	while ((inw(CONFIG_I2C_BASE + I2SR) & I2C_BB) && --timeout)
		udelay(1);

	outw(CONFIG_I2C_BASE + I2DR, byte);
	DPRINTF("tx_byte 0x%02x\n", byte);

	outw(CONFIG_I2C_BASE + I2CR, I2C_ENABLE | I2C_TBEN);
	DPRINTF("I2CR 0x%02x\n", inw(CONFIG_I2C_BASE + I2CR));

	if (!tx_wait_busy())
		return -1;

	return 0;
}

static int rx_wait_busy(void)
{
    int timeout = TIMEOUT;

	for (; timeout > 0; -- timeout)
	if (inw(CONFIG_I2C_BASE + I2SR) & I2C_DR)
		break;
	else
		udelay(1);

	return timeout;
}

static int rx_byte(int num)
{
	if(num)
		outw(CONFIG_I2C_BASE + I2CR, I2C_ENABLE | I2C_TBEN);
	else
		outw(CONFIG_I2C_BASE + I2CR, I2C_ENABLE | I2C_TBEN | I2C_STOP | I2C_ACKNAK);

	DPRINTF("I2CR 0x%02x\n", inw(CONFIG_I2C_BASE + I2CR));

	if (!rx_wait_busy())
		return -1;

	return inw(CONFIG_I2C_BASE + I2DR);
}


/* mode: read = 1, write = 0 */
static int i2c_addr(uint mode, uchar chip, uint addr, int alen)
{
	if(mode){
		outw(CONFIG_I2C_BASE + I2DR, chip | mode);
		DPRINTF("tx_byte 0x%02x\n", chip | mode);
	}
	else{
		outw(CONFIG_I2C_BASE + I2DR, chip & 0xFE);
		DPRINTF("tx_byte 0x%02x\n", chip & 0xFE);
	}

	outw(CONFIG_I2C_BASE + I2CR, I2C_ENABLE | I2C_TBEN | I2C_START);
	DPRINTF("I2CR 0x%02x\n", inw(CONFIG_I2C_BASE + I2CR));

	if (!tx_wait_busy())
		return -1;

	if(!mode){
		while (alen--)
			if (tx_byte((addr >> (alen * 8)) & 0xff))
				return -1;
	}
	return 0;
}

int i2c_read(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	int ret;

	DPRINTF("%s chip: 0x%02x addr: 0x%04x alen: %d len: %d\n",__FUNCTION__, chip, addr, alen, len);

	if (alen > 1) {
		printf ("I2C read: addr len %d not supported\n", alen);
		return 1;
	}

	if (addr + len > 256) {
		printf ("I2C : address out of range\n");
		return 1;
	}

	if (i2c_addr(0, chip, addr, alen)) {
		printf("i2c_addr failed\n");
		return -1;
	}

	if (i2c_addr(1, chip, addr, alen)) {
		printf("i2c_addr failed\n");
		return -1;
	}

    if(len == 0) { // Also issue 1-byte read even when "len == 0" to complete an I2C transaction
        ret = rx_byte(len);
        if(ret == -1) // This fixed the wrong result of "i2c probe" command
            return -1;
    }
    else {
	    while (len--) {
		    if ((ret = rx_byte(len)) < 0)
			    return -1;

		    DPRINTF("rx_byte 0x%02x\n", ret);
		    *buf++ = ret;
	    }
	}
	return 0;
}

int i2c_write(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	DPRINTF("%s chip: 0x%02x addr: 0x%04x alen: %d len: %d\n",__FUNCTION__, chip, addr, alen, len);

	if (alen > 1) {
		printf ("I2C write: addr len %d not supported\n", alen);
		return 1;
	}

	if (addr + len > 256) {
		printf ("I2C : address out of range\n");
		return 1;
	}

	if (i2c_addr(0, chip, addr, alen)){
		printf("i2c_addr failed\n");
		return -1;
	}

	while (len--)
		if (tx_byte(*buf++))
			return -1;

	outw(CONFIG_I2C_BASE + I2CR, I2C_ENABLE | I2C_TBEN | I2C_STOP);
	DPRINTF("I2CR 0x%02x\n", inw(CONFIG_I2C_BASE + I2CR));

	if (!tx_wait_busy())
		return -1;

	return 0;
}

int i2c_probe(uchar chip)
{
	return i2c_read(chip, 0, 0, NULL, 0);
}

#endif /* CONFIG_HARD_I2C */
