#include <c8051f340.h>
#include <stddef.h>
#include "USB_API.h"
#include "I2C_header.h"
#include "HDMI_header.h"


void TI_apply_setting (unsigned char format) {
	if (P1 & 0x02) return;

	switch (format & 0x7F)
	{
		case VID_02_720x480p:
			TI_board_setup_480p();
			break;

		case VID_04_1280x720p:
			TI_board_setup_720p();
			break;

		case VID_05_1920x1080i:
			TI_board_setup_1080i();
			break;

		case VID_16_1920x1080p:			// 1080p 60Hz
			TI_board_setup_1080p();
			break;

		default:
			break;
	}

	// Write D3 80
	I2C_ByteWrite (0xD3, 0x80);
}

void TI_board_setup_480p (void) {
	TI_ByteWrite (0x01,0x6b);  // Comments: PLL DIVMSB     1716   
	TI_ByteWrite (0x02,0x40);  // Comments: PLL DIVLSB            
	TI_ByteWrite (0x03,0x68);  // Comments: VCO2_CP3_RR_CP_R      
	TI_ByteWrite (0x04,0xb1);  // Comments: PHASE SEL(5) CKDI CKDI
	TI_ByteWrite (0x05,0x06);  // Comments: CLAMP START           
	TI_ByteWrite (0x06,0x10);  // Comments: CLAMP WIDTH 
	TI_ByteWrite (0x0F,0x26);  // Comments: PLL and CLAMP CONTROL 
	TI_ByteWrite (0x10,0x85);  // Comments: SOG Threshold-(YPbPr C
	TI_ByteWrite (0x11,0x47);  // Comments: SYNC SEPERATOR THRESHO
	TI_ByteWrite (0x12,0x03);  // Comments: PRE_COAST             
	TI_ByteWrite (0x13,0x0C);  // Comments: POST_COAST            
	TI_ByteWrite (0x15,0x02);  // Comments: Output Formater
	TI_ByteWrite (0x16,0x25);  // Comments: MISC Control          
	TI_ByteWrite (0x19,0x00);  // Comments: INPUT MUX SELECT    CH
	TI_ByteWrite (0x1A,0x85);  // Comments: INPUT MUX SELECT  HSYN
	TI_ByteWrite (0x1E,0x1F);  // Comments: COARSE OFFSET BLUE    
	TI_ByteWrite (0x1F,0x1F);  // Comments: COARSE OFFSET GREEN   
	TI_ByteWrite (0x20,0x1F);  // Comments: COARSE OFFSET BLUE    
	TI_ByteWrite (0x21,0x08);  // Comments: HSOUT START
	TI_ByteWrite (0x22,0x08);  // Comments: Macrovision support   
	TI_ByteWrite (0x26,0x80);  // Comments: ALC RED and GREEN LSB 
	TI_ByteWrite (0x28,0x03);  // Comments: AL FILTER Control     
	TI_ByteWrite (0x2A,0x07);  // Comments: Enable FINE CLAMP CONT
	TI_ByteWrite (0x2B,0x00);  // Comments: POWER CONTROL-SOG ON
	TI_ByteWrite (0x2C,0x50);  // Comments: ADC Setup
	TI_ByteWrite (0x2D,0x00);  // Comments: Coarse Clamp OFF
	TI_ByteWrite (0x2E,0x80);  // Comments: SOG Clamp ON
	TI_ByteWrite (0x31,0x18);  // Comments: ALC PLACEMENT 
	TI_ByteWrite (0x07,0x60);  // Comments: HSYNC OUTPUT WIDTH - 9
	TI_ByteWrite (0x0E,0x04);  // Comments: SYNC CONTROL    HSout+
}

void TI_board_setup_720p (void) {
	TI_ByteWrite (0x01,0x67);  // Comments: PLL DIVMSB  1650
	TI_ByteWrite (0x02,0x20);  // Comments: PLL DIVLSB                  
	TI_ByteWrite (0x03,0xA8);  // Comments: VCO2_CP3_RR_CP_R            
	TI_ByteWrite (0x04,0xb0);  // Comments: PHASE SEL(5) CKDI CKDI DIV2 
	TI_ByteWrite (0x05,0x32);  // Comments: CLAMP START                 
	TI_ByteWrite (0x06,0x20);  // Comments: CLAMP WIDTH 
	TI_ByteWrite (0x0F,0x2E);  // Comments: PLL and CLAMP CONTROL 
	TI_ByteWrite (0x10,0x85);  // Comments: SOG Threshold-(YPbPr Clamp) 
	TI_ByteWrite (0x11,0x47);  // Comments: SYNC SEPERATOR THRESHOLD    
	TI_ByteWrite (0x12,0x00);  // Comments: PRE_COAST                   
	TI_ByteWrite (0x13,0x00);  // Comments: POST_COAST                  
	TI_ByteWrite (0x16,0x25);  // Comments: MISC Control                
	TI_ByteWrite (0x19,0x00);  // Comments: INPUT MUX SELECT    CH1 sele
	TI_ByteWrite (0x1A,0x85);  // Comments: INPUT MUX SELECT  HSYNC_B an
	TI_ByteWrite (0x1E,0x1F);  // Comments: COARSE OFFSET BLUE          
	TI_ByteWrite (0x1F,0x1F);  // Comments: COARSE OFFSET GREEN         
	TI_ByteWrite (0x20,0x1F);  // Comments: COARSE OFFSET BLUE          
	TI_ByteWrite (0x21,0x31);  // Comments: HSOUT START 
	TI_ByteWrite (0x22,0x08);  // Comments: Macrovision support         
	TI_ByteWrite (0x26,0x80);  // Comments: ALC RED and GREEN LSB       
	TI_ByteWrite (0x28,0x03);  // Comments: AL FILTER Control           
	TI_ByteWrite (0x2A,0x07);  // Comments: Enable FINE CLAMP CONTROL
	TI_ByteWrite (0x2B,0x00);  // Comments: POWER CONTROL-SOG ON
	TI_ByteWrite (0x2C,0x50);  // Comments: ADC Setup
	TI_ByteWrite (0x2D,0x00);  // Comments: Coarse Clamp OFF
	TI_ByteWrite (0x2E,0x80);  // Comments: SOG Clamp ON
	TI_ByteWrite (0x31,0x5A);  // Comments: ALC PLACEMENT 
	TI_ByteWrite (0x07,0x28);  // Comments: HSYNC OUTPUT WIDTH - 40
	TI_ByteWrite (0x0E,0x20);  // Comments: SYNC CONTROL    HSout+ 
	TI_ByteWrite (0x15,0x06);  // Comments: OUTPUT FORMATTER, 422, CrCb
}

void TI_board_setup_1080i (void) {
	TI_ByteWrite (0x01,0x89);  // Comments: PLL DIVMSB   2200          
	TI_ByteWrite (0x02,0x80);  // Comments: PLL DIVLSB                 
	TI_ByteWrite (0x03,0xA8);  // Comments: PLL CONTROL                
	TI_ByteWrite (0x04,0xa0);  // Comments: PHASE SEL(5) CKDI CKDI DIV2
	TI_ByteWrite (0x05,0x32);  // Comments: CLAMP START                
	TI_ByteWrite (0x06,0x20);  // Comments: CLAMP WIDTH 
	TI_ByteWrite (0x0F,0x2E);  // Comments: PLL and CLAMP CONTROL 
	TI_ByteWrite (0x10,0x85);  // Comments: SOG Threshold-(YPbPr Clamp)
	TI_ByteWrite (0x11,0x47);  // Comments: SYNC SEPERATOR THRESHOLD   
	TI_ByteWrite (0x12,0x03);  // Comments: PRE_COAST                  
	TI_ByteWrite (0x13,0x00);  // Comments: POST_COAST                 
	TI_ByteWrite (0x16,0x25);  // Comments: MISC Control               
	TI_ByteWrite (0x19,0x00);  // Comments: INPUT MUX SELECT    CH1 sel
	TI_ByteWrite (0x1A,0x85);  // Comments: INPUT MUX SELECT  HSYNC_B a
	TI_ByteWrite (0x1E,0x1F);  // Comments: COARSE OFFSET BLUE         
	TI_ByteWrite (0x1F,0x1F);  // Comments: COARSE OFFSET GREEN        
	TI_ByteWrite (0x20,0x1F);  // Comments: COARSE OFFSET BLUE         
	TI_ByteWrite (0x21,0x35);  // Comments: HSOUT START
	TI_ByteWrite (0x22,0x0A);  // Comments: Macrovision support, VS ALIG
	TI_ByteWrite (0x26,0x80);  // Comments: ALC RED and GREEN LSB      
	TI_ByteWrite (0x28,0x03);  // Comments: AL FILTER Control          
	TI_ByteWrite (0x2A,0x07);  // Comments: Enable FINE CLAMP CONTROL
	TI_ByteWrite (0x2B,0x00);  // Comments: POWER CONTROL-SOG ON
	TI_ByteWrite (0x2C,0x50);  // Comments: ADC Setup
	TI_ByteWrite (0x2D,0x00);  // Comments: Coarse Clamp OFF
	TI_ByteWrite (0x2E,0x80);  // Comments: SOG Clamp ON
	TI_ByteWrite (0x31,0x5A);  // Comments: ALC PLACEMENT 
	TI_ByteWrite (0x07,0x2C);  // Comments: HSYNC OUTPUT WIDTH - 44
	TI_ByteWrite (0x0E,0x20);  // Comments: SYNC CONTROL    HSout+ 
	TI_ByteWrite (0x15,0x06);  // Comments: OUTPUT FORMATTER, 422, CrCb
}

void TI_board_setup_1080p (void) {
	TI_ByteWrite (0x01,0x89);  // Comments: PLL DIVMSB   2200          
	TI_ByteWrite (0x02,0x80);  // Comments: PLL DIVLSB                 
	TI_ByteWrite (0x03,0xD8);  // Comments: PLL CONTROL                
	TI_ByteWrite (0x04,0xC0);  // Comments: PHASE SEL(5) CKDI CKDI DIV2
	TI_ByteWrite (0x05,0x32);  // Comments: CLAMP START                
	TI_ByteWrite (0x06,0x20);  // Comments: CLAMP WIDTH 
	TI_ByteWrite (0x0F,0x2E);  // Comments: PLL and CLAMP CONTROL 
	TI_ByteWrite (0x10,0x85);  // Comments: SOG Threshold-(YPbPr Clamp)
	TI_ByteWrite (0x11,0x47);  // Comments: SYNC SEPERATOR THRESHOLD   
	TI_ByteWrite (0x12,0x00);  // Comments: PRE_COAST                  
	TI_ByteWrite (0x13,0x00);  // Comments: POST_COAST                 
	TI_ByteWrite (0x16,0x25);  // Comments: MISC Control               
	TI_ByteWrite (0x19,0x00);  // Comments: INPUT MUX SELECT    CH1 sel
	TI_ByteWrite (0x1A,0x85);  // Comments: INPUT MUX SELECT  HSYNC_B a
	TI_ByteWrite (0x1E,0x1F);  // Comments: COARSE OFFSET BLUE         
	TI_ByteWrite (0x1F,0x1F);  // Comments: COARSE OFFSET GREEN        
	TI_ByteWrite (0x20,0x1F);  // Comments: COARSE OFFSET BLUE         
	TI_ByteWrite (0x21,0x08);  // Comments: HSOUT START
	TI_ByteWrite (0x22,0x0A);  // Comments: Macrovision support, VS ALIG
	TI_ByteWrite (0x26,0x80);  // Comments: ALC RED and GREEN LSB      
	TI_ByteWrite (0x28,0x03);  // Comments: AL FILTER Control          
	TI_ByteWrite (0x2A,0x07);  // Comments: Enable FINE CLAMP CONTROL
	TI_ByteWrite (0x2B,0x00);  // Comments: POWER CONTROL-SOG ON
	TI_ByteWrite (0x2C,0x50);  // Comments: ADC Setup
	TI_ByteWrite (0x2D,0x00);  // Comments: Coarse Clamp OFF
	TI_ByteWrite (0x2E,0x80);  // Comments: SOG Clamp ON
	TI_ByteWrite (0x31,0x5A);  // Comments: ALC PLACEMENT 
	TI_ByteWrite (0x07,0x60);  // Comments: HSYNC OUTPUT WIDTH - 44
	TI_ByteWrite (0x0E,0x20);  // Comments: SYNC CONTROL    HSout+ ### ORIGINAL
	//TI_ByteWrite (0x0E,0x21);  // Comments: SYNC CONTROL    HSout+ 
	TI_ByteWrite (0x15,0x06);  // Comments: OUTPUT FORMATTER, 422, CrCb
}
