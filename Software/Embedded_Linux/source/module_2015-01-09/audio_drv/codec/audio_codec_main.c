#define TEST
//#define DISPLAY_PCM
//#define DISPLAY_ENCODE_DATA
//#define DISPLAY_TICK_INFO


//#include <sys/time.h>
//#include <stdio.h>
//#include <stdint.h>
//#include <stdlib.h>
//#include <string.h>
#include <linux/string.h>
#include <linux/init.h>

#include "audio_main.h"

//----- AAC codec API ----------------
#include "gmaac_enc_api.h"
#include "gmaac_dec_api.h"
//----- end --------------------------


//-------- ADPCM codec API -----------
#include "adpcm.h"
#include "ima_adpcm_enc_api.h"
#include "ima_adpcm_dec_api.h"
//------------------------------------

//-------- a/u law codec API -----------
#include "g711s.h" 
#include "filter.h"
//------------------------------------

//-------- audio coprocessor ---------
#include "fa410.h"
//-------- end -----------------------

#ifdef TEST

//struct timeval {
//    time_t      tv_sec;     /* seconds */
//    suseconds_t tv_usec;    /* microseconds */
//};

//struct timezone {
//    int tz_minuteswest;     /* minutes west of Greenwich */
//    int tz_dsttime;         /* type of DST correction */
//};

//#include <cyg/hal/hal_io.h>
#ifdef DISPLAY_TICK_INFO
struct timeval  tv;
struct timezone tz;
#endif
//#include <cyg/hal/fie702x.h>
//#include <cyg/kernel/kapi.h>
//#include <cyg/io/io.h>
//#include <cyg/io/pmu/fie702x_pmu.h>
#include "in_output_data_table.h"
#endif




#ifdef TEST
int frame_num = 0;
short         inputBuf[MaxMultiChannel][32768];
unsigned char outputBuf[MaxMultiChannel][16384];
#endif

#ifdef TEST
//unsigned char adpcm_outfile[MaxMultiChannel][8192+60];
short    decOutputBuf[505];
unsigned char decInputBuf[256];
#endif

#ifdef TEST
unsigned short tick_info[32];
unsigned short tick_cnt = 0;
cyg_tick_count_t aacStartTick, aacEndTick;
#endif

//------ AAC encoder use ----------------------------------
short  		encInputBuf[MaxMultiChannel][2048];   //stereo mode
unsigned char 	encOutputBuf[MaxMultiChannel][2048];
unsigned int    outputLen[MaxMultiChannel];
unsigned int    encLen[MaxMultiChannel];
//------ end -----------------------------------------------
//unsigned char bitstream_chk_buf[2048];

void advance_buffer(aac_buffer *b, int bytes)
{
    //b->file_offset += bytes;
    b->bytes_consumed = bytes;
    b->bytes_into_buffer -= bytes;
}

void aac_decode_fill_buf(aac_buffer *b, unsigned char **in_ptr, int *EncDataSize)
{
	unsigned char *inptr;
	inptr = *in_ptr;
	if(b->bytes_consumed > 0)
	{								//4608 is full
	    int fill_size;
            if (b->bytes_into_buffer)
		memmove((void*)b->buffer, (void*)(b->buffer + b->bytes_consumed), b->bytes_into_buffer*sizeof(unsigned char));
	    if(*EncDataSize != 0)
	    {
		fill_size = DECODE_BUF_SIZE - b->bytes_into_buffer;
		if(*EncDataSize > fill_size)
		{
		    memcpy((void*)(b->buffer+b->bytes_into_buffer), inptr, fill_size);
		    *EncDataSize -= fill_size;
		    b->bytes_into_buffer += fill_size;
		    inptr += fill_size;
		}
		else
		{
		    memcpy((void*)(b->buffer+b->bytes_into_buffer), inptr, *EncDataSize);
		    b->bytes_into_buffer += *EncDataSize;
		    *EncDataSize = 0;
		}
	    }
	    b->bytes_consumed = 0;
	}
	else if( (b->bytes_consumed == 0)&&(*EncDataSize!=0)&&(b->bytes_into_buffer==0) ) //initial state
	{
		if(*EncDataSize > DECODE_BUF_SIZE)
		{
		    memcpy((void*)b->buffer, inptr, DECODE_BUF_SIZE);
		    *EncDataSize -= DECODE_BUF_SIZE;
		    b->bytes_into_buffer = DECODE_BUF_SIZE;
		    inptr += DECODE_BUF_SIZE;
		}
		else
		{
		    memcpy((void*)b->buffer, inptr, *EncDataSize);
		    b->bytes_into_buffer = *EncDataSize;
		    *EncDataSize = 0;
		}
	}
    *in_ptr = inptr;
}

int auto_mode_chk_audio_type(unsigned char **dec_in_ptr, unsigned int EncDataSize, unsigned int *skip_data)
{
	int i;
	unsigned char  *op_ptr;
	int got_correct_frame_header = 0;
	unsigned int frame_length;
        unsigned char  id, layer, sr_idx;

	op_ptr = *dec_in_ptr;
	for(i=0;i<EncDataSize;i++)
	{
	    if( (unsigned short)(op_ptr[0]<<8 | (op_ptr[1]&0xf0)) == 0xfff0)
	    {
	    	id = 	(op_ptr[1]&0x8)>>3;
	    	layer = (op_ptr[1]&0x6)>>1;
	    	sr_idx= (op_ptr[2]&0x3c)>>2;
	    	if( (id!=1)||(layer!=0)||(sr_idx>11) )
	    	    op_ptr = op_ptr+1;
	    	else
	    	{
	    	    frame_length = ((op_ptr[3]&0x3)<<11) | (op_ptr[4]<<3) | ((op_ptr[5]&0xe0)>>5);
	    	    //if( (j+frame_length)> op_size )
	    	    //   got_correct_frame_header = 1;
	    	    //else
	    	    //{
	    	    	if ( (*(op_ptr + frame_length)==0xff)&&((*(op_ptr + frame_length+1)&0xf0)==0xf0) )
	    	    	{
			    got_correct_frame_header = 1;
			    break;
			}
	    	    	else
	    	    	    op_ptr = op_ptr+1;
	    	    //}
	    	}
	    }
	    else
	        op_ptr = op_ptr+1;		//shift one byte
	}

    *dec_in_ptr = op_ptr;
    *skip_data = i;
    return got_correct_frame_header;
}

void main(int argc, char *argv[])
{

	int i, j, count_i, test_loop;
	unsigned int pcmDataSize, decoded_length;
	int decode_ch, InputSampleNumber;
	int ch;

	//********************* AAC CODEC parameters ******************************
	GMAAC_INFORMATION aacEncInfo;
	//*************************************************************************

	//********************* ADPCM CODEC parameters ******************************
	IMA_state_t *state[MaxMultiChannel][2];
	//***************************************************************************

	//********************* g.711 CODEC parameters ******************************
	INPARAM *inparam_enc[MaxMultiChannel], *inparam_dec;
	FILTER *bandpass;
	//unsigned short aulaw_enc_option, aulaw_dec_option;
	//***************************************************************************
	
	//********************* AAC Decode parameters *******************************
  	GMAACDecHandle hDecoder;
	GMAACDecInfo *frameInfo;
	aac_buffer b;
	void *sample_buffer;
#ifdef DISPLAY_PCM
	short *out_buf_ptr;				//output is "short" format
#endif
	unsigned int EncDataSize;
	unsigned char *dec_in_ptr;
	//********************* end *************************************************

	AUDIO_CODEC_SETTING codec_setting;


//************************************************************************************
//ADPCM CODEC Setting	
//*************************************************************************************
	codec_setting.ima_paras.nBlockAlign = 256;
	codec_setting.ima_paras.nChannels = 2;
//**** end *****************************************************************************		
	
//************************************************************************************
// Encoder setting:
//************************************************************************************
	codec_setting.encode_audio_type = 1;			//0: AAC 1:ADPCM 2:AULAW
	codec_setting.EncMultiChannel =  16; 			//max support to 32-ch
	codec_setting.encode_error = 0;
	if( (codec_setting.EncMultiChannel > 0)&&(codec_setting.encode_audio_type==0) )
		for(i=0; i<codec_setting.EncMultiChannel; i++)
		{	//---------- aac parameters ------------------
			codec_setting.aac_enc_paras[i].sampleRate = 8000;
			codec_setting.aac_enc_paras[i].bitRate    = 14500;
			codec_setting.aac_enc_paras[i].nChannels  = 1;
			//------------ end ----------------------------

		}
    //---------- adpcm parameters -----------------
    //codec_setting.ima_enc_paras.nChannels   = 1;
    //codec_setting.ima_enc_paras.nBlockAlign = BLOCKALIGN;
    //codec_setting.ima_enc_paras.wSamplesPerBlock = IMA_BLK_SZ;
    //---------- end ------------------------------
//************************************************************************************

//************************************************************************************
// Decoder setting:
//************************************************************************************
	codec_setting.decode_error = 0;
	codec_setting.decode_mode =  2;			//0: AUTO: 1:AAC 2:ADPCM 3:aulaw
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//**********************************************************************
//G711 codec setting
	codec_setting.aulaw_codec_pars.aulaw_enc_option = LINEAR2ALAW;
	codec_setting.aulaw_codec_pars.aulaw_dec_option = ALAW2LINEAR;
	codec_setting.aulaw_codec_pars.block_size = AULAW_BLK_SZ;
	codec_setting.aulaw_codec_pars.Decoded_Sample_Num = AULAW_BLK_SZ;
//**********************************************************************
#ifdef TEST
	//for(test_loop = 0; test_loop<4; test_loop++)
	//{
	    if(codec_setting.encode_audio_type == 0)
	    {
		pcmDataSize = 32768;
		decoded_length = 0;
		for(count_i=0; count_i< codec_setting.EncMultiChannel ; count_i++)
		{
			outputLen[count_i] = 0;
			for (i = 0; i < pcmDataSize; i++)
			{
				//if( (count_i&0x1) == 0)
				  inputBuf[count_i][i]= in_data_8k_1ch[i];//input samples:  length = 40960 (frame num: 40)
				//else if( (count_i&0x1) == 1)
				  //inputBuf[count_i][itest_loop*8192]= in_data_16k_1ch[i+test_loop*8192];//input samples: length = 40000 (frame num: 39)
			}
		}
	    }
	    else if(codec_setting.encode_audio_type == 1)
	    {
	    	pcmDataSize = (codec_setting.ima_paras.nChannels==1)?(IMA_BLK_SZ_MONO*64):(IMA_BLK_SZ_DUAL*64);
		for(ch = 0; ch<codec_setting.EncMultiChannel; ch++)
		{
		    outputLen[ch] = 0;
		    for(i=0; i<pcmDataSize; i++)
		    {
		    	  if( codec_setting.ima_paras.nChannels==1 )
			    inputBuf[ch][i] = in_data_8k_1ch[i];
			  else	  
			      		inputBuf[ch][i] = in_data_8k_2ch[i];
			//else
			    //inputBuf[ch][i] = in_data_16k_1ch[i+test_loop*pcmDataSize];
		    }
		}
	    }
	    else if(codec_setting.encode_audio_type == 2)
	    {
	    	pcmDataSize = 8192;
		for(ch = 0; ch<codec_setting.EncMultiChannel; ch++)
		{
		    outputLen[ch] = 0;
		    for(i=0; i<pcmDataSize; i++)
		    {
		    	//if( (ch&1) == 0 )
#ifndef TEST_DC_REJECT
			    inputBuf[ch][i]= in_data_8k_1ch_aulaw[i];				
#else
			    inputBuf[ch][i]= in_data_8k_1ch_aulaw[i] + 2000;
                         printf("%hx\n",  inputBuf[ch][i]);
#endif		    	
			//else
			    //inputBuf[ch][i] = in_data_16k_1ch[i+test_loop*pcmDataSize];	    
		    }
		}	
	    }		
	//}
#endif



//*******************************************************************************************************************
// Function: Audio Encoder
// Support Type: ADPCM, AAC
//*******************************************************************************************************************


//*******************************************************************************************************************
// 	Init Encoder:
//*******************************************************************************************************************
#ifdef DISPLAY_TICK_INFO
	gettimeofday(&tv, &tz);
    	aacStartTick = tv.tv_sec*1000000+tv.tv_usec;
#endif
	switch(codec_setting.encode_audio_type)
	{
		case AAC_ENCODE:
    		     if (AAC_Create(1 /* ID, 0 for MPEG-4, 1 for MPEG-2*/, &aacEncInfo, &codec_setting, codec_setting.EncMultiChannel) > 0)
        		 codec_setting.encode_error = 1;
		     InputSampleNumber = AAC_FRAME_SZ;
        	     break;
		case IMA_ADPCM_ENCODE:
		     if(Init_IMA_ADPCM_Enc(&codec_setting.ima_paras, state, codec_setting.EncMultiChannel)!=0)
		 	 codec_setting.encode_error = 1;
		     //InputSampleNumber = IMA_BLK_SZ;
		     InputSampleNumber = codec_setting.ima_paras.Sample_Num;
		     break;
		case AULAW_ENCODE:
		     if(g711_init_enc(&codec_setting, inparam_enc)!=0)
		 	 codec_setting.encode_error = 1;
		     if( init_bandpass_filter(&bandpass, codec_setting.EncMultiChannel)!=0)
	   		 codec_setting.encode_error = 1;	 
		     InputSampleNumber = AULAW_BLK_SZ;	 
		     break;
		default:
		     break;
	}
#ifdef DISPLAY_TICK_INFO
        gettimeofday(&tv, &tz);
    	aacEndTick = tv.tv_sec*1000000+tv.tv_usec;
    	printk("AAC_Create() ticks: %d\n", (aacEndTick - aacStartTick)/1000);/*tick_info[tick_cnt++] = aacEndTick - aacStartTick;*/
#endif



//*******************************************************************************************************************
// 	Encode PCM:
//*******************************************************************************************************************


#ifdef TEST
	for (i = 0;i < (pcmDataSize/InputSampleNumber);i ++)
	{
	         int ch;
		 for (ch = 0;ch < codec_setting.EncMultiChannel;ch ++)
		     memcpy((void *)encInputBuf[ch], (void *)(inputBuf[ch] + i*InputSampleNumber), sizeof(short)*InputSampleNumber);
#ifdef DISPLAY_TICK_INFO
		 if( (i&7)==0 )
		 {
        		gettimeofday(&tv, &tz);
    			aacStartTick = tv.tv_sec*1000000+tv.tv_usec;
        	 }
#endif
#endif
		 switch(codec_setting.encode_audio_type)
		 {
		 	case AAC_ENCODE:
		             if (AAC_Enc(encInputBuf, encOutputBuf, encLen, &aacEncInfo) > 0)
		                 codec_setting.encode_error = 1;//printk("AAC_Enc() failed\n");
		 	     break;

		 	case IMA_ADPCM_ENCODE:
		 	     if (IMA_ADPCM_Enc(state, encInputBuf, encOutputBuf, encLen, codec_setting.EncMultiChannel)> 0)
		 	         codec_setting.encode_error = 1;
		 	    break;
				
		 	case AULAW_ENCODE:
			    bandpass_filter(bandpass, encInputBuf, encInputBuf, codec_setting.aulaw_codec_pars.block_size, codec_setting.EncMultiChannel);	
#ifdef TEST_DC_REJECT				 
			    {
			        int j;
				 for(j=0;j<codec_setting.aulaw_codec_pars.block_size;j++)
				 	printf("%hx\n",encInputBuf[0][j]);
			      
			    }
#endif
		 	     if (G711_Enc(inparam_enc, encInputBuf, encOutputBuf, encLen, codec_setting.EncMultiChannel)> 0)
		 	         codec_setting.encode_error = 1;
		 	    break;				
		 	default:
		 	    break;
		 }
#ifdef DISPLAY_TICK_INFO
		if( (i&7)==7 )
		{
        	    gettimeofday(&tv, &tz);
    		    aacEndTick = tv.tv_sec*1000000+tv.tv_usec;
		    printk("Encode ticks loop %d: %d\n", i, (aacEndTick - aacStartTick)/1000);//tick_info[tick_cnt++] = aacEndTick - aacStartTick;
		}
#endif
#ifdef TEST
	         for (ch = 0;ch < codec_setting.EncMultiChannel;ch ++)
	         {
	              memcpy((void *)(outputBuf[ch] + outputLen[ch]), (void *)encOutputBuf[ch], encLen[ch]);
	              outputLen[ch] += encLen[ch];
	         }
	}
#endif
#ifdef DISPLAY_ENCODE_DATA
	for(ch=0; ch<codec_setting.EncMultiChannel; ch++)
	    for(j=0;j<outputLen[ch];j++)
	      printk("%x\n",outputBuf[ch][j]);
#endif
//*******************************************************************************************************************



//*******************************************************************************************************************
// 	Destory Encoder:
//*******************************************************************************************************************
	switch(codec_setting.encode_audio_type)
	{
		case AAC_ENCODE:
		     AAC_Destory(&aacEncInfo);
		     break;
		case IMA_ADPCM_ENCODE:
        	     //Update_IMA_ADPCM_File_Header(adpcm_header, outputLen, codec_setting.EncMultiChannel);
#ifdef TEST
		     //for(j = 0; j<codec_setting.EncMultiChannel; j++)
		     //{
		     //	memmove(outputBuf[j]+ADPCM_HEAD_LEN, outputBuf[j], outputLen[j]);
		     //	memcpy(outputBuf[j], adpcm_header[j], ADPCM_HEAD_LEN);
		     //}
		     //for(i=0; i<codec_setting.EncMultiChannel; i++)
		     //    printk("encoded length:%d\n", 60+outputLen[i]);

		     //for(i=0; i<codec_setting.EncMultiChannel; i++)
		     //{
		     //    for(j=0;j<(60+outputLen[i]);j++)
		     //	printk("%x\n", adpcm_outfile[i][j]);
		     //}
		     //fread(&adpcm_outfile[0],1,4096+60,infile);
        	     //dec_file_ptr = adpcm_outfile[1];
#endif
		     IMA_ADPCM_Enc_Destory(state, codec_setting.EncMultiChannel);
		     break;
	 	case AULAW_ENCODE:
	 	     g711enc_destory(inparam_enc, codec_setting.EncMultiChannel);
		     filter_destory(bandpass);	 
	 	     break;			 
		default:
		     break;
	}
//*******************************************************************************************************************


//*******************************************************************************************************************
// Function: Audio Decoder
// Support Type: ADPCM, AAC
//*******************************************************************************************************************
#ifdef TEST
	for(i=0;i<(codec_setting.EncMultiChannel-1);i++)
	{
		if( outputLen[i] !=outputLen[i+1])
			outputLen[i] +=1;
	}		
	for(i=0;i<outputLen[0];i++)
	{
	    for(j=0;j<(codec_setting.EncMultiChannel-1);j++)
	    {
		  if( outputBuf[j][i]!= outputBuf[j+1][i] )
			outputBuf[j][i] += 1;
	    }		
	}
	decode_ch = 0;
        EncDataSize = outputLen[decode_ch];
	dec_in_ptr = outputBuf[decode_ch];
#endif



//*******************************************************************************************************************
// Judge bitstream type if decode mode is "AUTO":
// input bitstream pointer: 	dec_in_ptr
// bitstream length:		EncDataSize
//*******************************************************************************************************************
#ifdef TEST
        //------- ROBUST TEST ----------------------------------
        //EncDataSize = 7572;
        //for(i=0;i<7572;i++)
        //    outputBuf[0][i] = encoded_pattern[i];
        //------------------------------------------------------
#endif
        if( codec_setting.decode_mode == 0 )
        {
            unsigned int preceeding_garbage_data;
            if(auto_mode_chk_audio_type(&dec_in_ptr, EncDataSize, &preceeding_garbage_data)==1)//if return "0", none aac frame header found
            {
               	EncDataSize -= preceeding_garbage_data;		     //if found, dec_in_ptr pointer to frame start addr
               	codec_setting.decode_audio_type = 0;
            }
            else
            {
	        codec_setting.decode_error = 1;
	        codec_setting.decode_audio_type = -1;
            }
        }
        else
            codec_setting.decode_audio_type = codec_setting.decode_mode - 1;							     //EncDataSize --> skip preceeding garbage data
//*******************************************************************************************************************


//*******************************************************************************************************************
// 	Init Decoder:
//*******************************************************************************************************************
	switch(codec_setting.decode_audio_type)
	{
	 	case AAC_DECODE:
		     if(GM_AAC_Dec_Init(&hDecoder, &b, &frameInfo) !=0 )
   		    	 codec_setting.decode_error = 1;
	   	     break;
	   	case IMA_ADPCM_DECODE:
		     if(Init_IMA_ADPCM_Dec(&codec_setting.ima_paras) !=0 )
   		     	 codec_setting.decode_error = 1;
	   	     break;
		case AULAW_DECODE:
		     if(g711_init_dec(&codec_setting, &inparam_dec)!=0)
		 	 codec_setting.encode_error = 1;
	   	     break;		 	    	
		default:
		     break;
	}

//*******************************************************************************************************************
// 	Decode:
//*******************************************************************************************************************
#ifdef DISPLAY_TICK_INFO
        gettimeofday(&tv, &tz);
    	aacStartTick = tv.tv_sec*1000000+tv.tv_usec;
#endif

#ifdef TEST
	while(1)
	{
            if(codec_setting.decode_audio_type == AAC_DECODE)
            {
            	if( (EncDataSize == 0)&&(b.bytes_into_buffer==0) )
            	    break;
	    	aac_decode_fill_buf(&b, &dec_in_ptr, &EncDataSize); 	//buffer size is 4608 bytes
            }
            else if(codec_setting.decode_audio_type == IMA_ADPCM_DECODE)
            {
            	if(EncDataSize >= codec_setting.ima_paras.nBlockAlign)
            	{
        	    for(i=0; i<codec_setting.ima_paras.nBlockAlign;i++)
        	    	decInputBuf[i] = *dec_in_ptr++;

        	    EncDataSize -= codec_setting.ima_paras.nBlockAlign;	
        	}
        	else
        	    break;
            }
            else if(codec_setting.decode_audio_type == AULAW_DECODE)
            {
            	if(EncDataSize >= codec_setting.aulaw_codec_pars.block_size)
            	{
        	    for(i=0; i<codec_setting.aulaw_codec_pars.block_size;i++)
        	    	decInputBuf[i] = *dec_in_ptr++;      
        	    	      	
        	    EncDataSize -= codec_setting.aulaw_codec_pars.block_size;	
        	}
		else		
			break;
            }
            
        	    

#endif
	    switch(codec_setting.decode_audio_type)
	    {
	 	case AAC_DECODE:
	 	         //printk("enter AAC decode\n");
			 sample_buffer = GMAACDecode(hDecoder, frameInfo, b.buffer, b.bytes_into_buffer);
        		 if ((frameInfo->error == 0) && (frameInfo->samples > 0))
			 {
#ifdef DISPLAY_PCM
				out_buf_ptr = (short *)sample_buffer;
				for (i=0; i<frameInfo->samples; i++)
				    printk("%hx\n", (unsigned short)*(out_buf_ptr+i));
#endif
			 }
			 advance_buffer(&b, frameInfo->bytesconsumed);
			 break;
	 	case IMA_ADPCM_DECODE:
		        IMA_ADPCM_dec(&codec_setting.ima_paras, decInputBuf, decOutputBuf);
#ifdef DISPLAY_PCM
		        for(j=0;j<codec_setting.ima_paras.Sample_Num;j++)
 			{
                    printf("%hx\n",(unsigned short)decOutputBuf[j]);
                }
#endif
                break;
	 	case AULAW_DECODE:
		        G711_Dec(inparam_dec, decInputBuf, decOutputBuf);
#ifdef DISPLAY_PCM
		        for(j=0;j<codec_setting.aulaw_codec_pars.Decoded_Sample_Num;j++)
 			{
			    	printf("%hx\n",(unsigned short)decOutputBuf[j]);
			}
#endif
	 	        break;
		default:
		        break;
		 }
	} //while(1)
#ifdef DISPLAY_TICK_INFO
        gettimeofday(&tv, &tz);
    	aacEndTick = tv.tv_sec*1000000+tv.tv_usec;
    	printk("Decode ticks: %d\n", (aacEndTick - aacStartTick)/1000);//tick_info[tick_cnt++] = aacEndTick - aacStartTick;
#endif
//*******************************************************************************************************************
// 	Destory Decoder:
//*******************************************************************************************************************
	switch(codec_setting.decode_audio_type)
	{
	 	case AAC_DECODE:
		     GM_AAC_Dec_Destory(hDecoder, &b);
	   	     break;
	   	case IMA_ADPCM_DECODE:
	   	     break;
	 	case AULAW_DECODE:			 
		     g711dec_destory(inparam_dec);
		     break; 
		default:
		     break;
	}
//*******************************************************************************************************************

//	if(tick_cnt != 0)
//	{
//		for(i=0;i<tick_cnt; i++)
//			printk("tick_info = %d\n", (unsigned short)tick_info[i]);
//
//	}
    return 0;
}
static void __exit audio_codec_test_exit(void)
{
}
module_init(audio_codec_test_init);
module_exit(audio_codec_test_exit);
