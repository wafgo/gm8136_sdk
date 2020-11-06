#include <linux/stddef.h>
#include <linux/string.h>
#include "adpcm.h"
#include "audio_main.h"

#if 0
void IMA_ADPCM_Dec_Destory(header_t *adpcm_header)
{
	if(adpcm_header)
	{
		  fact_ParmSetNULL(&adpcm_header->fact);		/* 2*256-7 = 512-7 = 505 */
		  data_ParmSetNULL(&adpcm_header->data);
		   wav_ParmSetNULL(&adpcm_header->wav);
		format_ParmSetNULL(&adpcm_header->format);
		Aligmem_free(adpcm_header);
		adpcm_header = NULL;
	}
	return 0;

}



int Init_IMA_ADPCM_Dec(header_t **adpcm_header_dec_, unsigned char **file_ptr)
{
	char id[4];
	list_chunk_t	list;
	unsigned char *dec_file_ptr = *file_ptr;
	header_t *adpcm_header_dec;
	int error = 0;

	    adpcm_header_dec = (header_t *)Alignm_mem(sizeof(header_t), 32);/*(header_t *)malloc(sizeof(header_t))*/;

	    memcpy(&adpcm_header_dec->wav, dec_file_ptr, sizeof(wav_file_t));
	    dec_file_ptr += sizeof(wav_file_t);
	    memcpy(&adpcm_header_dec->format, dec_file_ptr, sizeof(format_chunk_t));
	    dec_file_ptr += sizeof(format_chunk_t);
     	    while(1){
		//fread(id, 4*sizeof(char), 1, ifp);
		//fseek(ifp, -4, SEEK_CUR);
		memcpy(id, dec_file_ptr, 4);
		if(0==strncmp(id, "data", 4)){
			memcpy(&adpcm_header_dec->data, dec_file_ptr, sizeof(data_chunk_t));
			dec_file_ptr += sizeof(data_chunk_t);
			//fread(&data, sizeof(data_chunk_t), 1, ifp);
		}else if(0==strncmp(id, "LIST", 4)){
			memcpy(&list, dec_file_ptr, sizeof(list_chunk_t));
			dec_file_ptr += sizeof(list_chunk_t);
			dec_file_ptr += list.Info_len;
			//fread(&list, sizeof(list_chunk_t), 1, ifp);
			//fseek(ifp, list.Info_len, SEEK_CUR);
		}else if(0==strncmp(id, "fact", 4)){
			memcpy(&adpcm_header_dec->fact, dec_file_ptr, sizeof(fact_chunk_t));
			dec_file_ptr += sizeof(fact_chunk_t);
			//fread(&fact, sizeof(fact_chunk_t), 1, ifp);
		}else{
			break;
		}
	    }
	    *file_ptr = dec_file_ptr;
	    *adpcm_header_dec_ = adpcm_header_dec;
	    if(	((adpcm_header_dec->wav.id[0]!=0x52)&&(adpcm_header_dec->wav.id[1]!=0x49)&&(adpcm_header_dec->wav.id[2]!=0x46)&&(adpcm_header_dec->wav.id[3]!=0x46))||(adpcm_header_dec->wav.len==0) )
	        error = 1;
	    if( (adpcm_header_dec->format.wFormatTag!=0x11)||((adpcm_header_dec->format.nChannels!=1)&&(adpcm_header_dec->format.nChannels!=2)) )
	        error = 1;
	    if( adpcm_header_dec->data.Data_len== 0 )
	        error = 1;
	return  error;
}
#endif

int Init_IMA_ADPCM_Dec(ADPCM_CODEC_PARAMETERS *ima_paras)
{
    if(ima_paras->nChannels > 2)
        return 1;
    if(ima_paras->nBlockAlign == 0)
        ima_paras->nBlockAlign = BLOCKALIGN;

	ima_paras->Sample_Num = (ima_paras->nBlockAlign - (ima_paras->nChannels<<2))*2 + ima_paras->nChannels;

    return 0;
}
EXPORT_SYMBOL(Init_IMA_ADPCM_Dec);


void IMA_ADPCM_dec(ADPCM_CODEC_PARAMETERS *ima_paras, unsigned char decInputBuf[], short decOutputBuf[])
{
    	IMA_state_t state[2];
    	INT16U Channels, MaxChannels;
	INT32U BlockAlign;
	INT8U ibyte;
	short *pcm_ptr;
	unsigned char *in_ptr;
	INT16S pcm[MAXCHANNELS][8];
	char nibble;
	int i,j,m,n;

	pcm_ptr = &decOutputBuf[0];
	in_ptr = &decInputBuf[0];

	Channels=ima_paras->nChannels;
	BlockAlign=ima_paras->nBlockAlign;
	MaxChannels=(Channels>MAXCHANNELS)?MAXCHANNELS:Channels;
	//if(MaxChannels<Channels)
	//    return 1;

	for(i=0; i<MaxChannels; i++){
		memcpy(&state[i], in_ptr, sizeof(IMA_state_t));
		in_ptr += sizeof(IMA_state_t);
		*pcm_ptr++ = state[i].prev_val;
		//if(0==fread(&state[i], sizeof(IMA_state_t), 1, ifp)){
		//	goto finish;
		//}
		//fprintf(ofp, "%d\n", state[i].prev_val);
	}

	//BlockCnt++;
	//if(MaxChannels<Channels)
	//	return 1;//fseek(ifp, (Channels-MaxChannels)*sizeof(IMA_state_t), SEEK_CUR);

	for(j=0; j<((BlockAlign/Channels)-4)/4; j++){
		for(i=0; i<MaxChannels; i++){
			ibyte = *in_ptr++;//fread(&ibyte, sizeof(INT8U), 1, ifp);
			nibble= ibyte & 0xf;
			pcm[i][0]=adpcm_decode(&state[i], nibble);

			nibble= (ibyte & 0xf0) >> 4;
			pcm[i][1]=adpcm_decode(&state[i], nibble);

			ibyte = *in_ptr++;//fread(&ibyte, sizeof(INT8U), 1, ifp);
			nibble= ibyte & 0xf;
			pcm[i][2]=adpcm_decode(&state[i], nibble);

			nibble= (ibyte & 0xf0) >> 4;
			pcm[i][3]=adpcm_decode(&state[i], nibble);


			ibyte = *in_ptr++;//fread(&ibyte, sizeof(INT8U), 1, ifp);
			nibble= ibyte & 0xf;
			pcm[i][4]=adpcm_decode(&state[i], nibble);

			nibble= (ibyte & 0xf0) >> 4;
			pcm[i][5]=adpcm_decode(&state[i], nibble);

			ibyte = *in_ptr++;//fread(&ibyte, sizeof(INT8U), 1, ifp);
			nibble= ibyte & 0xf;
			pcm[i][6]=adpcm_decode(&state[i], nibble);

			nibble= (ibyte & 0xf0) >> 4;
			pcm[i][7]=adpcm_decode(&state[i], nibble);

		}
		for(n=0;n<8;n++)
		    for(m=0; m<MaxChannels; m++)
				*pcm_ptr++ = pcm[m][n];

	}
    //*Decoded_Sample_Num = adpcm_header_dec->format.wSamplesPerBlock*adpcm_header_dec->format.nChannels;
    //return 0;
}
EXPORT_SYMBOL(IMA_ADPCM_dec);


/*
int Count_Block_Num(header_t *adpcm_header_dec)
{
	return (adpcm_header_dec->data.Data_len/adpcm_header_dec->format.nBlockAlign);
}
*/
