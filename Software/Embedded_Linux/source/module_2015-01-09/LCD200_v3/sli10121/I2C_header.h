#ifndef  _I2C_HEADER_H_
#define  _I2C_HEADER_H_


//-----------------------------------------------------------------------------
// Global CONSTANTS - From SMBus_EEPROM
//-----------------------------------------------------------------------------


#define SYSCLK                   12000000    // SYSCLK frequency in Hz
#define  SMB_FREQUENCY  100000          // Target SCL clock rate (100kHz)
                                       // This example supports between 10kHz
                                       // and 100kHz

#define  WRITE          0x00           // SMBus WRITE command
#define  READ           0x01           // SMBus READ command

// Device addresses (7 bits, lsb is a don't care)
#define  I2C_ADDR    0x76           // Device address for slave target
#define  I2C_TI_ADDR    0xB8           // Device address for slave target
                                       // Note: This address is specified
                                       // in the SLI HDMI datasheet
// SMBus Buffer Size
#define  SMB_BUFF_SIZE  0x08           // Defines the maximum number of bytes
                                       // that can be sent or received in a
                                       // single transfer

// Status vector - top 4 bits only
#define  SMB_MTSTA      0xE0           // (MT) start transmitted
#define  SMB_MTDB       0xC0           // (MT) data byte transmitted
#define  SMB_MRDB       0x80           // (MR) data byte received
// End status vector definition


// Function Prototypes for SMBus
void SMBus_Init(void);
void Timer1_Init(void);
void Timer3_Init(void);
void SMBus_ISR(void);
void Timer3_ISR(void);

void I2C_ByteWrite(unsigned char addr, unsigned char dat);
unsigned char I2C_ByteRead(unsigned char addr);
void I2C_ReadArray (unsigned char* dest_addr, unsigned char src_addr, unsigned char len);

void TI_ByteWrite(unsigned char addr, unsigned char dat);

unsigned char I2C_register_test(void);
void Delay(void);
void DelayMs (BYTE millisecond);

#endif  /* _I2C_HEADER_H_ */
