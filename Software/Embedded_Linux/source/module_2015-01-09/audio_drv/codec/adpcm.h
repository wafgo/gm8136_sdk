/*
** adpcm.h - include file for adpcm coder.
**
** Version 1.0, 7-Jul-92.
*/
#ifndef _ADPCM_
#define _ADPCM_

 
#define MAXCHANNELS	2

#define BLOCKALIGN      256

typedef unsigned int INT32U;
typedef int INT32S;

typedef unsigned short INT16U;
typedef short INT16S;

typedef unsigned char INT8U;
typedef char INT8S;


typedef struct IMA_state{
    INT16S	prev_val;        /* Previous output value */
    INT8U	index;          /* Index into stepsize table */
    INT8U	reserved;
}IMA_state_t;


typedef struct wav_file{
	INT8U	id[4];			//4
	INT32U	len;			//4
	INT8U	Wave_id[4];		//4
}wav_file_t;				//total: 12 


typedef struct format_chunk{
	INT8U	Fmt_id[4];		//4
	INT32U	Fmt_len;		//4
	INT16U	wFormatTag;		//2
	INT16U	nChannels;		//2
	INT32U	nSamplePerSec;		//4
	INT32U	nAvgBytesPerSec;	//4
	INT16U	nBlockAlign;		//2
	INT16U	wBitsPerSample;		//2
	INT16U	cbSize;			//2
	INT16U	wSamplesPerBlock;	//2
}format_chunk_t;			//total = 28

typedef struct fact_chunk{
	INT8U	Fact_id[4];		//4
	INT32U	Fact_len;		//4
	INT32U	dwFileSize;		//4
}fact_chunk_t;				//total: 12

typedef struct list_chunk{
	INT8U	List_id[4];
	INT32U	List_len;
	INT8U	Info_id[8];
	INT32U	Info_len;
}list_chunk_t;


typedef struct data_chunk{
	INT8U	Data_id[4];		//4
	INT32U	Data_len;		//4
}data_chunk_t;				//sum: 8


typedef struct header{
	wav_file_t	wav;		//12
	format_chunk_t	format;		//28
	fact_chunk_t	fact;		//12
	data_chunk_t	data;		//8
}header_t;				//sum: 60



short adpcm_encode(IMA_state_t *state, short pcm);

short adpcm_decode(IMA_state_t *state, char nibble);
void IMA_init_state(struct IMA_state *);

void fact_ParmSetNULL(fact_chunk_t *hfact);
void data_ParmSetNULL(data_chunk_t *hdata);
void wav_ParmSetNULL(wav_file_t *hwav);
void format_ParmSetNULL(format_chunk_t *hformat);
#endif