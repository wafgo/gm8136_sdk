#include <c8051f340.h>
#include <stddef.h>
#include "USB_API.h"
#include "I2C_header.h"
#include "HDMI_header.h"


//-----------------------------------------------------------------------------
// Global Externs
//-----------------------------------------------------------------------------
/*
#define Sw1 0x01                          // These are the port2 bits for Sw1
#define Sw2 0x02                          // and Sw2 on the development board
extern BYTE Switch1State;                    // Indicate status of switch
extern BYTE Switch2State;                    // starting at 0 == off

extern BYTE Toggle1;                         // Variable to make sure each button
extern BYTE Toggle2;                         // press and release toggles switch
*/

//-----------------------------------------------------------------------------
// 16-bit SFR Definitions for 'F32x
//-----------------------------------------------------------------------------

sfr16    TMR3RL   = 0x92;              // Timer3 reload registers
sfr16    TMR3     = 0x94;              // Timer3 counter registers

sbit Led1 = P2^0;
sbit Led2 = P2^1;                         // LED='1' means ON
sbit Led3 = P2^2;
sbit Led4 = P2^3;
sbit Led5 = P2^4;
sbit Led6 = P2^5;
sbit Led7 = P2^6;

//-----------------------------------------------------------------------------
// Global VARIABLES - From SMBus_EEPROM
//-----------------------------------------------------------------------------

//sbit SDA = P0^6;                       // SMBus on P0.6
//sbit SCL = P0^7;                       // and P0.7

unsigned char* pSMB_DATA_IN;           // Global pointer for SMBus data
                                       // All receive data is written here

unsigned char SMB_SINGLEBYTE_OUT;      // Global holder for single byte writes.

unsigned char* pSMB_DATA_OUT;          // Global pointer for SMBus data.
                                       // All transmit data is read from here

unsigned char SMB_DATA_LEN;            // Global holder for number of bytes
                                       // to send or receive in the current
                                       // SMBus transfer.

unsigned char WORD_ADDR;               // Global holder for the EEPROM word
                                       // address that will be accessed in
                                       // the next transfer

unsigned char TARGET;                  // Target SMBus slave address

bit SMB_BUSY = 0;                      // Software flag to indicate when the
                                       // I2C_ByteRead() or
                                       // I2C_ByteWrite()
                                       // functions have claimed the SMBus

bit SMB_RW;                            // Software flag to indicate the
                                       // direction of the current transfer

bit SMB_SENDWORDADDR;                  // When set, this flag causes the ISR
                                       // to send the 8-bit <WORD_ADDR>
                                       // after sending the slave address.


bit SMB_RANDOMREAD;                    // When set, this flag causes the ISR
                                       // to send a START signal after sending
                                       // the word address.
                                       // For the 24LC02B EEPROM, a random read
                                       // (a read from a particular address in
                                       // memory) starts as a write then
                                       // changes to a read after the repeated
                                       // start is sent. The ISR handles this
                                       // switchover if the <SMB_RANDOMREAD>
                                       // bit is set.

bit SMB_ACKPOLL;                       // When set, this flag causes the ISR
                                       // to send a repeated START until the
                                       // slave has acknowledged its address

//-----------------------------------------------------------------------------
// Global VARIABLES - I2C Reigster Read/Write Command
//-----------------------------------------------------------------------------
/*
BYTE cmdRead_req = 0;
BYTE cmdRead_addr = 0;
BYTE cmdRead_value = 0;
BYTE cmdWrite_req = 0;
BYTE cmdWrite_addr = 0;
BYTE cmdWrite_value = 0;
*/

//-----------------------------------------------------------------------------
// Initialization Routines - From SMBus_EEPROM
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// SMBus_Init()
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// The SMBus peripheral is configured as follows:
// - SMBus enabled
// - Slave mode disabled
// - Timer1 used as clock source. The maximum SCL frequency will be
//   approximately 1/3 the Timer1 overflow rate
// - Setup and hold time extensions enabled
// - Free and SCL low timeout detection enabled
//
void SMBus_Init (void)
{
   SMB0CF = 0x5D;                      // Use Timer1 overflows as SMBus clock
                                       // source;
                                       // Disable slave mode;
                                       // Enable setup & hold time extensions;
                                       // Enable SMBus Free timeout detect;
                                       // Enable SCL low timeout detect;

   SMB0CF |= 0x80;                     // Enable SMBus;

   EIE1 |= 0x01;                       // Enable SMBus interrupt;
   //EIP1 |= 0x01;                       // Set SMBus interrupt priority high; ###
}

//-----------------------------------------------------------------------------
// Timer1_Init()
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Timer1 is configured as the SMBus clock source as follows:
// - Timer1 in 8-bit auto-reload mode
// - SYSCLK / 12 as Timer1 clock source
// - Timer1 overflow rate => 3 * SMB_FREQUENCY
// - The maximum SCL clock rate will be ~1/3 the Timer1 overflow rate
// - Timer1 enabled
//
void Timer1_Init (void)
{
// Make sure the Timer can produce the appropriate frequency in 8-bit mode
// Supported SMBus Frequencies range from 10kHz to 100kHz.  The CKCON register
// settings may need to change for frequencies outside this range.
#if ((SYSCLK/SMB_FREQUENCY/3) < 255)
   #define SCALE 1
      CKCON |= 0x08;                   // Timer1 clock source = SYSCLK
#elif ((SYSCLK/SMB_FREQUENCY/4/3) < 255)
   #define SCALE 4
      CKCON |= 0x01;
      CKCON &= ~0x0A;                  // Timer1 clock source = SYSCLK / 4
#endif

   TMOD = 0x20;                        // Timer1 in 8-bit auto-reload mode

   TH1 = -(SYSCLK/SMB_FREQUENCY/12/3); // Timer1 configured to overflow at 1/3
                                       // the rate defined by SMB_FREQUENCY

   TL1 = TH1;                          // Init Timer1

   TR1 = 1;                            // Timer1 enabled
}

//-----------------------------------------------------------------------------
// Timer3_Init()
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Timer3 configured for use by the SMBus low timeout detect feature as
// follows:
// - Timer3 in 16-bit auto-reload mode
// - SYSCLK/12 as Timer3 clock source
// - Timer3 reload registers loaded for a 25ms overflow period
// - Timer3 pre-loaded to overflow after 25ms
// - Timer3 enabled
//
void Timer3_Init (void)
{
   TMR3CN = 0x00;                      // Timer3 configured for 16-bit auto-
                                       // reload, low-byte interrupt disabled

   CKCON &= ~0x40;                     // Timer3 uses SYSCLK/12

   TMR3RL = -(SYSCLK/12/40);           // Timer3 configured to overflow after
   TMR3 = TMR3RL;                      // ~25ms (for SMBus low timeout detect)

   EIE1 |= 0x80;                       // Timer3 interrupt enable
   TMR3CN |= 0x04;                     // Start Timer3


   //EIP1 |= 0x80;                       // Set Timer3 priority high ###
}



//-----------------------------------------------------------------------------
// SMBus Interrupt Service Routine (ISR)
//-----------------------------------------------------------------------------
//
// SMBus ISR state machine
// - Master only implementation - no slave or arbitration states defined
// - All incoming data is written starting at the global pointer <pSMB_DATA_IN>
// - All outgoing data is read from the global pointer <pSMB_DATA_OUT>
//
void SMBus_ISR (void) interrupt 7
{
   bit FAIL = 0;                       // Used by the ISR to flag failed
                                       // transfers

   static char i;                      // Used by the ISR to count the
                                       // number of data bytes sent or
                                       // received

   static bit SEND_START = 0;          // Send a start

   switch (SMB0CN & 0xF0)              // Status vector
   {
      // Master Transmitter/Receiver: START condition transmitted.
      case SMB_MTSTA:
         SMB0DAT = TARGET;             // Load address of the target slave
         SMB0DAT &= 0xFE;              // Clear the LSB of the address for the
                                       // R/W bit
         SMB0DAT |= SMB_RW;            // Load R/W bit
         STA = 0;                      // Manually clear START bit
         i = 0;                        // Reset data byte counter
         break;

      // Master Transmitter: Data byte (or Slave Address) transmitted
      case SMB_MTDB:
         if (ACK)                      // Slave Address or Data Byte
         {                             // Acknowledged?
            if (SEND_START)
            {
               STA = 1;
               SEND_START = 0;
               break;
            }
            if(SMB_SENDWORDADDR)       // Are we sending the word address?
            {
               SMB_SENDWORDADDR = 0;   // Clear flag
               SMB0DAT = WORD_ADDR;    // Send word address

               if (SMB_RANDOMREAD)
               {
                  SEND_START = 1;      // Send a START after the next ACK cycle
                  SMB_RW = READ;
               }

               break;
            }

            if (SMB_RW==WRITE)         // Is this transfer a WRITE?
            {

               if (i < SMB_DATA_LEN)   // Is there data to send?
               {
                  // send data byte
                  SMB0DAT = *pSMB_DATA_OUT;

                  // increment data out pointer
                  pSMB_DATA_OUT++;

                  // increment number of bytes sent
                  i++;
               }
               else
               {
                 STO = 1;              // Set STO to terminte transfer
                 SMB_BUSY = 0;         // Clear software busy flag
               }
            }
            else {}                    // If this transfer is a READ,
                                       // then take no action. Slave
                                       // address was transmitted. A
                                       // separate 'case' is defined
                                       // for data byte recieved.
         }
         else                          // If slave NACK,
         {
            if(SMB_ACKPOLL)
            {
               STA = 1;                // Restart transfer
            }
            else
            {
               FAIL = 1;               // Indicate failed transfer
            }                          // and handle at end of ISR
         }
         break;

      // Master Receiver: byte received
      case SMB_MRDB:
         if ( i < SMB_DATA_LEN )       // Is there any data remaining?
         {
            *pSMB_DATA_IN = SMB0DAT;   // Store received byte
            pSMB_DATA_IN++;            // Increment data in pointer
            i++;                       // Increment number of bytes received
            ACK = 1;                   // Set ACK bit (may be cleared later
                                       // in the code)

         }

         if (i == SMB_DATA_LEN)        // This is the last byte
         {
            SMB_BUSY = 0;              // Free SMBus interface
            ACK = 0;                   // Send NACK to indicate last byte
                                       // of this transfer
            STO = 1;                   // Send STOP to terminate transfer
         }

         break;

      default:
         FAIL = 1;                     // Indicate failed transfer
                                       // and handle at end of ISR
         break;
   }

   if (FAIL)                           // If the transfer failed,
   {
      SMB0CF &= ~0x80;                 // Reset communication
      SMB0CF |= 0x80;
      STA = 0;
      STO = 0;
      ACK = 0;

      SMB_BUSY = 0;                    // Free SMBus

      FAIL = 0;
   }

   SI = 0;                             // Clear interrupt flag
}

//-----------------------------------------------------------------------------
// Timer3 Interrupt Service Routine (ISR)
//-----------------------------------------------------------------------------
//
// A Timer3 interrupt indicates an SMBus SCL low timeout.
// The SMBus is disabled and re-enabled if a timeout occurs.
//
void Timer3_ISR (void) interrupt 14
{
   SMB0CF &= ~0x80;                    // Disable SMBus
   SMB0CF |= 0x80;                     // Re-enable SMBus
   TMR3CN &= ~0x80;                    // Clear Timer3 interrupt-pending flag

   //STA = 0; //###
   SMB_BUSY = 0;                       // Free bus
   //while(1);
}

//-----------------------------------------------------------------------------
// Support Functions - From SMBus_EEPROM
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// I2C_ByteWrite ()
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   :
//   1) unsigned char addr - address to write in the EEPROM
//                        range is full range of character: 0 to 255
//
//   2) unsigned char dat - data to write to the address <addr> in the EEPROM
//                        range is full range of character: 0 to 255
//
// This function writes the value in <dat> to location <addr> in the EEPROM
// then polls the EEPROM until the write is complete.
//
void I2C_ByteWrite(unsigned char addr, unsigned char dat)
{
   while (SMB_BUSY);                   // Wait for SMBus to be free.
   SMB_BUSY = 1;                       // Claim SMBus (set to busy)

   // Set SMBus ISR parameters
   TARGET = I2C_ADDR;               // Set target slave address
   SMB_RW = WRITE;                     // Mark next transfer as a write
   SMB_SENDWORDADDR = 1;               // Send Word Address after Slave Address
   SMB_RANDOMREAD = 0;                 // Do not send a START signal after
                                       // the word address
   SMB_ACKPOLL = 1;                    // Enable Acknowledge Polling (The ISR
                                       // will automatically restart the
                                       // transfer if the slave does not
                                       // acknoledge its address.

   // Specify the Outgoing Data
   WORD_ADDR = addr;                   // Set the target address in the
                                       // EEPROM's internal memory space

   SMB_SINGLEBYTE_OUT = dat;           // Store <dat> (local variable) in a
                                       // global variable so the ISR can read
                                       // it after this function exits

   // The outgoing data pointer points to the <dat> variable
   pSMB_DATA_OUT = &SMB_SINGLEBYTE_OUT;

   SMB_DATA_LEN = 1;                   // Specify to ISR that the next transfer
                                       // will contain one data byte

   // Initiate SMBus Transfer
   STA = 1;


}
void TI_ByteWrite(unsigned char addr, unsigned char dat)
{
   while (SMB_BUSY);                   // Wait for SMBus to be free.
   SMB_BUSY = 1;                       // Claim SMBus (set to busy)

   // Set SMBus ISR parameters
   TARGET = I2C_TI_ADDR;               // Set target slave address
   SMB_RW = WRITE;                     // Mark next transfer as a write
   SMB_SENDWORDADDR = 1;               // Send Word Address after Slave Address
   SMB_RANDOMREAD = 0;                 // Do not send a START signal after
                                       // the word address
   SMB_ACKPOLL = 1;                    // Enable Acknowledge Polling (The ISR
                                       // will automatically restart the
                                       // transfer if the slave does not
                                       // acknoledge its address.

   // Specify the Outgoing Data
   WORD_ADDR = addr;                   // Set the target address in the
                                       // EEPROM's internal memory space

   SMB_SINGLEBYTE_OUT = dat;           // Store <dat> (local variable) in a
                                       // global variable so the ISR can read
                                       // it after this function exits

   // The outgoing data pointer points to the <dat> variable
   pSMB_DATA_OUT = &SMB_SINGLEBYTE_OUT;

   SMB_DATA_LEN = 1;                   // Specify to ISR that the next transfer
                                       // will contain one data byte

   // Initiate SMBus Transfer
   STA = 1;

}

//-----------------------------------------------------------------------------
// I2C_ByteRead ()
//-----------------------------------------------------------------------------
//
// Return Value :
//   1) unsigned char data - data read from address <addr> in the EEPROM
//                        range is full range of character: 0 to 255
//
// Parameters   :
//   1) unsigned char addr - address to read data from the EEPROM
//                        range is full range of character: 0 to 255
//
// This function returns a single byte from location <addr> in the EEPROM then
// polls the <SMB_BUSY> flag until the read is complete.
//
unsigned char I2C_ByteRead(unsigned char addr)
{
   unsigned char retval;               // Holds the return value

   while (SMB_BUSY);                   // Wait for SMBus to be free.
   SMB_BUSY = 1;                       // Claim SMBus (set to busy)

   // Set SMBus ISR parameters
   TARGET = I2C_ADDR;               // Set target slave address
   SMB_RW = WRITE;                     // A random read starts as a write
                                       // then changes to a read after
                                       // the repeated start is sent. The
                                       // ISR handles this switchover if
                                       // the <SMB_RANDOMREAD> bit is set.
   SMB_SENDWORDADDR = 1;               // Send Word Address after Slave Address
   SMB_RANDOMREAD = 1;                 // Send a START after the word address
   SMB_ACKPOLL = 1;                    // Enable Acknowledge Polling


   // Specify the Incoming Data
   WORD_ADDR = addr;                   // Set the target address in the
                                       // EEPROM's internal memory space

   pSMB_DATA_IN = &retval;             // The incoming data pointer points to
                                       // the <retval> variable.

   SMB_DATA_LEN = 1;                   // Specify to ISR that the next transfer
                                       // will contain one data byte

   // Initiate SMBus Transfer
   STA = 1;
   while(SMB_BUSY);                    // Wait until data is read

   return retval;

}

//-----------------------------------------------------------------------------
// I2C_ReadArray ()
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   :
//   1) unsigned char* dest_addr - pointer to the array that will be filled
//                                 with the data from the EEPROM
//                        range is full range of character: 0 to 255
//
//   2) unsigned char src_addr - beginning address to read data from the EEPROM
//                        range is full range of character: 0 to 255
//
//   3) unsigned char len - length of the array to be read from the EEPROM
//                        range is full range of character: 0 to 255
//
// Reads up to 256 data bytes from the EEPROM slave specified by the
// <EEPROM_ADDR> constant.
//
void I2C_ReadArray (unsigned char* dest_addr, unsigned char src_addr,
                       unsigned char len)
{
   while (SMB_BUSY);                   // Wait for SMBus to be free.
   SMB_BUSY = 1;                       // Claim SMBus (set to busy)

   // Set SMBus ISR parameters
   TARGET = I2C_ADDR;                  // Set target slave address
   SMB_RW = WRITE;                     // A random read starts as a write
                                       // then changes to a read after
                                       // the repeated start is sent. The
                                       // ISR handles this switchover if
                                       // the <SMB_RANDOMREAD> bit is set.
   SMB_SENDWORDADDR = 1;               // Send Word Address after Slave Address
   SMB_RANDOMREAD = 1;                 // Send a START after the word address
   SMB_ACKPOLL = 1;                    // Enable Acknowledge Polling

   // Specify the Incoming Data
   WORD_ADDR = src_addr;               // Set the target address in the
                                       // EEPROM's internal memory space

   // Set the the incoming data pointer
   pSMB_DATA_IN = (unsigned char*) dest_addr;


   SMB_DATA_LEN = len;                 // Specify to ISR that the next transfer
                                       // will contain <len> data bytes


   // Initiate SMBus Transfer
   STA = 1;
   while(SMB_BUSY);                    // Wait until data is read

}


void Delay (void)
{
	BYTE d;
	for (d = 0; d < 0xFF; d++)
		;
}

void DelayMs (BYTE millisecond)
{
	while (millisecond-- > 0)
		Delay();
}