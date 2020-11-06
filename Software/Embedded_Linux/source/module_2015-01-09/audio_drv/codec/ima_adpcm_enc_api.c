#include <linux/stddef.h>
#include <linux/string.h>
#include "adpcm.h"
#include "audio_main.h"
#include "mem_manag.h"

int Init_IMA_ADPCM_Enc(ADPCM_CODEC_PARAMETERS *ima_paras, IMA_state_t *state[][2], int MultiChannel)
{
	int i, j, error=0;

	//if(!codec_setting)
	//   return 1;

	//codec_setting->InputSampleNumber = 505;

    	if(ima_paras->nChannels > 2)
    	   return 1;	

	for(i=0;i<MultiChannel;i++)
		for(j=0;j<ima_paras->nChannels;j++)
	    		state[i][j] = /*(IMA_state_t *)malloc(sizeof(IMA_state_t));*/(IMA_state_t *)Alignm_mem(sizeof(IMA_state_t), 32);	

	for(i=0;i<MultiChannel;i++)
		for(j=0;j<ima_paras->nChannels;j++)
	    		state[i][j]->index=0;

	//if(codec_setting->ima_enc_paras.nChannels != 1)
	//    error = 1;

		
	if(ima_paras->nBlockAlign == 0)
	    ima_paras->nBlockAlign = BLOCKALIGN;
	
	ima_paras->Sample_Num = (ima_paras->nBlockAlign - (ima_paras->nChannels<<2))*2 + ima_paras->nChannels;
	
	return error;
}
EXPORT_SYMBOL(Init_IMA_ADPCM_Enc);



int IMA_ADPCM_Enc(IMA_state_t *state[][2], short encInputBuf[][2048], unsigned char encOutputBuf[][2048], int encLen[],  AUDIO_CODEC_SETTING *codec_setting)	
{
	int i, j, k,l;
	short *in_ptr;
	short *in_lch_ptr, *in_rch_ptr, *in_curr_ptr;
	unsigned char *out_ptr;
	char nibble;
	short BlockAlign, chs;
	char output;
	short Mulichannel_no;
	ADPCM_CODEC_PARAMETERS *adpcm_paras;
	//int Mulichannel_no;

	Mulichannel_no = codec_setting->EncMultiChannel;
	adpcm_paras = &codec_setting->ima_paras;
	BlockAlign=adpcm_paras->nBlockAlign;
	chs = adpcm_paras->nChannels;
	
	for(j=0; j<Mulichannel_no; j++)
	{
		in_ptr =  encInputBuf[j];
		out_ptr = encOutputBuf[j];
		if(adpcm_paras->nChannels == 1)
		{
		//BlockCnt++;
			state[j][0]->prev_val=*in_ptr++;//pcm;

			*out_ptr++ = state[j][0]->prev_val & 0x00ff;//fwrite(&state.prev_val, sizeof(INT16S), 1, ofp);
			*out_ptr++ = state[j][0]->prev_val>>8;
			*out_ptr++ = state[j][0]->index;		//fwrite(&state.index, sizeof(INT8U), 1, ofp);
		*out_ptr++ = 0;				//fwrite(&zero, sizeof(INT8U), 1, ofp);


			for(i=1; i<2*BlockAlign-7; i++)//504 samples for mono
		{
				nibble=adpcm_encode(state[j][0], *in_ptr++/*pcm*/);

			if(1==i%2){
				output = nibble;
			}else{
				output |= (nibble <<4);
				*out_ptr++ = output;//fwrite(&output, sizeof(INT8U), 1, ofp);
			}
		}
		}
		else
		{
			in_lch_ptr = in_ptr;
			in_rch_ptr = in_ptr + 1;
			//-------------- left channel ---------------------------------------------------	
			state[j][0]->prev_val=*in_lch_ptr;//pcm;
			in_lch_ptr += 2;
			*out_ptr++ = state[j][0]->prev_val & 0x00ff;//fwrite(&state.prev_val, sizeof(INT16S), 1, ofp);
			*out_ptr++ = state[j][0]->prev_val>>8;
			*out_ptr++ = state[j][0]->index;		//fwrite(&state.index, sizeof(INT8U), 1, ofp);
			*out_ptr++ = 0;				//fwrite(&zero, sizeof(INT8U), 1, ofp);
			//-------------- right channel ---------------------------------------------------	
			state[j][1]->prev_val=*in_rch_ptr;//pcm;
			in_rch_ptr += 2;
			*out_ptr++ = state[j][1]->prev_val & 0x00ff;//fwrite(&state.prev_val, sizeof(INT16S), 1, ofp);
			*out_ptr++ = state[j][1]->prev_val>>8;
			*out_ptr++ = state[j][1]->index;		//fwrite(&state.index, sizeof(INT8U), 1, ofp);
			*out_ptr++ = 0;				//fwrite(&zero, sizeof(INT8U), 1, ofp);			
			//-----------------------------------------------------------------------------
			l = (BlockAlign-8)>>3;
			for(i=1; i<=l; i++)//256 - 8 = 248, 248*2 = 496 samples, 248/8=31
			{
				for(k=0;k<chs;k++)
				{

					if(k==0)
						in_curr_ptr = in_lch_ptr;
					else
						in_curr_ptr = in_rch_ptr;
					
					nibble=adpcm_encode(state[j][k], *in_curr_ptr/*pcm*/);
					in_curr_ptr += 2;
					output = nibble;
					nibble=adpcm_encode(state[j][k], *in_curr_ptr/*pcm*/);
					in_curr_ptr += 2;
					output |= (nibble <<4);
					*out_ptr++ = output;
					nibble=adpcm_encode(state[j][k], *in_curr_ptr/*pcm*/);
					in_curr_ptr += 2;
					output = nibble;
					nibble=adpcm_encode(state[j][k], *in_curr_ptr/*pcm*/);
					in_curr_ptr += 2;
					output |= (nibble <<4);
					*out_ptr++ = output;	
					nibble=adpcm_encode(state[j][k], *in_curr_ptr/*pcm*/);
					in_curr_ptr += 2;
					output = nibble;
					nibble=adpcm_encode(state[j][k], *in_curr_ptr/*pcm*/);
					in_curr_ptr += 2;
					output |= (nibble <<4);
					*out_ptr++ = output;					
					nibble=adpcm_encode(state[j][k], *in_curr_ptr/*pcm*/);
					in_curr_ptr += 2;
					output = nibble;
					nibble=adpcm_encode(state[j][k], *in_curr_ptr/*pcm*/);
					in_curr_ptr += 2;
					output |= (nibble <<4);
					*out_ptr++ = output;					
				}
				in_lch_ptr += 16;
				in_rch_ptr += 16;
			}		
		}
		encLen[j] = BlockAlign;
	}
    return 0;
}
EXPORT_SYMBOL(IMA_ADPCM_Enc);



int IMA_ADPCM_Enc_Destory(IMA_state_t *state[][2], short multiChannel)
{
	int i,j;
	for(i=0;i<multiChannel;i++)
	{
	    if(state[i])
	    {
	       for(j=0;j<2;j++)
	       {
	       	if(state[i][j])
	       	{
				Aligmem_free(state[i][j]);
				state[i][j] = NULL;	    	
	       	}
	       }
		   
	    }
	}
	return 0;
}
EXPORT_SYMBOL(IMA_ADPCM_Enc_Destory);
#if 0
void Update_IMA_ADPCM_File_Header(header_t *adpcm_header[], unsigned int outputLen[], short MultiChannel)
{
	int ch;
	for(ch = 0; ch<MultiChannel; ch++)
	{
		adpcm_header[ch]->fact.dwFileSize=2*adpcm_header[ch]->format.nBlockAlign-7;		/* 2*256-7 = 512-7 = 505 */
		adpcm_header[ch]->data.Data_len=outputLen[ch];
		adpcm_header[ch]->wav.len=outputLen[ch]+60-8;
	}
}
#endif
