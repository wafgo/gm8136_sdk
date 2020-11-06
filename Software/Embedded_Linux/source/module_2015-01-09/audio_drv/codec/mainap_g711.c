#include <linux/string.h>
#include <linux/slab.h>
#include "g711s.h"

#define	ALAWBYTES	1
#define LINEARBYTES	2
#define	ULAWBYTES	1

//#define CHRI_DEBUG

/* do_convert */
int G711_Enc(INPARAM *inparam[], short encInputBuf[][2048], unsigned char encOutputBuf[][2048], int encLen[], short Mulichannel_no )
{
	int i, j;
	short *in_ptr;
	unsigned char *out_ptr;
	short BlockAlign;
	//char output;
	//int Mulichannel_no;

	for(j=0; j<Mulichannel_no; j++)
	{
		BlockAlign=inparam[j]->block_size;

		in_ptr =  encInputBuf[j];
		out_ptr = encOutputBuf[j];

		//for(i=0; i<inparam[j]->block_size;i++)
		//	(short)inparam[j]->input_buf[i] = *in_ptr++;
		memcpy(inparam[j]->input_buf, in_ptr, sizeof(short)*inparam[j]->block_size);

		g711_convertion(inparam[j], inparam[j]->option );

		for(i=0; i<inparam[j]->block_size;i++)
			*out_ptr++ = inparam[j]->output_buf[i];

		encLen[j] = inparam[j]->block_size;
	}
    return 0;
}
EXPORT_SYMBOL(G711_Enc);

int G711_Dec(INPARAM *inparam, unsigned char *InputBuf, short *OutputBuf )
{
	short *out_ptr;
	unsigned char *in_ptr;
	short BlockAlign;
	//char output;
	//int Mulichannel_no;

		BlockAlign=inparam->block_size;

		in_ptr =  InputBuf;
		out_ptr = OutputBuf;

		//for(i=0; i<inparam[j]->block_size;i++)
		//	(short)inparam[j]->input_buf[i] = *in_ptr++;
		memcpy(inparam->input_buf, in_ptr, inparam->block_size);

		g711_convertion(inparam, inparam->option );

		//for(i=0; i<inparam->block_size;i++)
		//	*out_ptr++ = inparam->output_buf[i];
		memcpy(out_ptr, inparam->output_buf, sizeof(short)*inparam->block_size);

    return 0;
}
EXPORT_SYMBOL(G711_Dec);

void g711_convertion(INPARAM *inparam, unsigned int option )
{

#ifdef CHRI_DEBUG
	printk( "Perform conversion...\n\n" ) ;
#endif

	switch( option ) {
#ifdef CHRI_DEBUG
		printk( "Converting...\n\n" ) ;
#endif
		case  1 :  // Convert linear PCM -> A-law
	   		 perform_conversion( inparam->input_buf, inparam->block_size, LINEARBYTES, ALAWBYTES, inparam->output_buf, G711_linear2alaw) ;
#ifdef CHRI_DEBUG
      printk( "Number of samples for conversion is '%d'\n\n", inparam->in_file_size/LINEARBYTES) ;
			printk( "Number of output samples is '%d'\n\n", inparam->out_file_size) ;
#endif
			break ;
		case  2 :  // Convert A-law      -> linear PCM
	    		 perform_conversion( inparam->input_buf,  inparam->block_size, ALAWBYTES, LINEARBYTES, inparam->output_buf, G711_alaw2linear) ;
#ifdef CHRI_DEBUG
      printk( "Number of samples for conversion is '%d'\n\n", inparam->in_file_size/ALAWBYTES) ;
			printk( "Number of output samples is '%d'\n\n", inparam->out_file_size) ;
#endif
			break ;
		case  3 :  // Convert linear PCM -> u-law
	    		 perform_conversion( inparam->input_buf, inparam->block_size, LINEARBYTES, ULAWBYTES, inparam->output_buf, G711_linear2ulaw) ;
#ifdef CHRI_DEBUG
      printk( "Number of samples for conversion is '%d'\n\n", inparam->in_file_size/LINEARBYTES) ;
			printk( "Number of output samples is '%d'\n\n", inparam->out_file_size) ;
#endif
			break ;
		case  4 :  // Convert u-law      -> linear PCM
	    		 perform_conversion( inparam->input_buf,  inparam->block_size, ULAWBYTES, LINEARBYTES, inparam->output_buf, G711_ulaw2linear) ;
#ifdef CHRI_DEBUG
      printk( "Number of samples for conversion is '%d'\n\n", inparam->in_file_size/ULAWBYTES) ;
			printk( "Number of output samples is '%d'\n\n", inparam->out_file_size) ;
#endif
			break ;
		case  5 :  // Convert A-law       -> u-law
	   		perform_conversion( inparam->input_buf,  inparam->block_size, ALAWBYTES, ULAWBYTES, inparam->output_buf, G711_alaw2ulaw) ;
#ifdef CHRI_DEBUG
      printk( "Number of samples for conversion is '%d'\n\n", inparam->in_file_size/ALAWBYTES) ;
			printk( "Number of output samples is '%d'\n\n", inparam->out_file_size) ;
#endif
			break ;
		case  6 :  // Convert u-law       -> A-law
	    		perform_conversion( inparam->input_buf,  inparam->block_size, ULAWBYTES, ALAWBYTES, inparam->output_buf, G711_ulaw2alaw) ;
#ifdef CHRI_DEBUG
      printk( "Number of samples for conversion is '%d'\n\n", inparam->in_file_size/ULAWBYTES) ;
			printk( "Number of output samples is '%d'\n\n", inparam->out_file_size) ;
#endif
			break ;
		case  0 :
		default :
			kfree( ( void * )inparam->input_buf ) ;
			printk( "Stop.\n\n" ) ;
			return;
	}

#ifdef CHRI_DEBUG
	printk( "Done.\n\n" ) ;
#endif
	return;
}

/* perform_conversion */
#if 0
char *perform_conversion( char inputs[ ], unsigned int numberBytes, unsigned int inBytes, unsigned int outBytes, unsigned int *numberBytesOut, int ( *ConvRoutine )( int ) )
{
	unsigned char	*ucinputs = ( unsigned char * )inputs ;
	unsigned int	numberInSamples ;
	char			    *outputs ;
	unsigned char	*ucoutputs ;
	unsigned int	counter ;
	int				    input ;
	int				    output ;

	if( ( !inputs ) || ( !numberBytesOut ) || ( !ConvRoutine ) ) {
		fprintk( stderr, "[perform_conversion] Error in input arguments, aborting.\n\n" ) ;
		/* function name given since intended as internal error for programmer */
		return NULL ;
	}

	numberInSamples = numberBytes/inBytes ;
	*numberBytesOut = numberInSamples * outBytes ;

	if( ( outputs = ( char * )calloc( *numberBytesOut, sizeof( char ) ) ) == NULL ) {
		fprintk( stderr, "Failure to create array for output data, aborting.\n\n" ) ;
		*numberBytesOut = -1 ;
		return NULL ;
	}
	ucoutputs = ( unsigned char * )outputs ;

	for( counter = 0 ; counter < numberInSamples ; counter += 1 ) {
		switch( inBytes ) {
			case 1 :
				input = ucinputs[ counter ] ;
				input <<= 24 ;
				input >>= 24 ;
				break ;
			case 2 :
				input = ucinputs[ counter << 1 ] + ( ucinputs[ ( counter << 1 ) + 1 ] << 8 ) ;
				input <<= 16 ;
				input >>= 16 ;
				break ;
			case 4 :
				input = ucinputs[ counter << 2 ] + ( ucinputs[ ( counter << 2 ) + 1 ] << 8 )
						+ ( ucinputs[ ( counter << 2 ) + 2 ] << 16 )
						+ ( ucinputs[ ( counter << 2 ) + 3 ] << 24 ) ;
				break ;
			default :
				free( ( void * ) outputs ) ;
				return NULL ;
		}

		output = ConvRoutine( input ) ;

		switch( outBytes ) {
			case 1 :
				ucoutputs[ counter ] = ( unsigned char )output ;
				break ;
			case 2 :
				ucoutputs[ ( counter << 1 ) ] = ( unsigned char )output ;
				output >>= 8 ;
				ucoutputs[ ( counter << 1 ) + 1 ] = ( unsigned char )output ;
				break ;
			case 4 :
				ucoutputs[ ( counter << 2 ) ] = ( unsigned char )output ;
				output >>= 8 ;
				ucoutputs[ ( counter << 2 ) + 1 ] = ( unsigned char )output ;
				output >>= 8 ;
				ucoutputs[ ( counter << 2 ) + 2 ] = ( unsigned char )output ;
				output >>= 8 ;
				ucoutputs[ ( counter << 2 ) + 3 ] = ( unsigned char )output ;
				break ;
			default :
				break ;
		}
	}

	return outputs ;
}
#else
void perform_conversion( char inputs[ ], unsigned short numberInSamples, unsigned int inBytes, unsigned int outBytes, char outputs[], int ( *ConvRoutine )( int ) )
{
	unsigned char	*ucinputs = ( unsigned char * )inputs ;
	//unsigned int	numberInSamples ;
	//char			    *outputs ;
	unsigned char	*ucoutputs ;
	unsigned int	counter ;
	int				    input ;
	int				    output ;

#if 0
	if( ( !inputs ) || ( !numberBytesOut ) || ( !ConvRoutine ) ) {
		fprintk( stderr, "[perform_conversion] Error in input arguments, aborting.\n\n" ) ;
		/* function name given since intended as internal error for programmer */
		return NULL ;
	}

	numberInSamples = numberBytes/inBytes ;
	*numberBytesOut = numberInSamples * outBytes ;

	if( ( outputs = ( char * )calloc( *numberBytesOut, sizeof( char ) ) ) == NULL ) {
		fprintk( stderr, "Failure to create array for output data, aborting.\n\n" ) ;
		*numberBytesOut = -1 ;
		return NULL ;
	}
#endif
	ucoutputs = ( unsigned char * )outputs ;


	for( counter = 0 ; counter < numberInSamples ; counter += 1 ) {
		switch( inBytes ) {
			case 1 :
				input = ucinputs[ counter ] ;
				input <<= 24 ;
				input >>= 24 ;
				break ;
			case 2 :
				input = ucinputs[ counter << 1 ] + ( ucinputs[ ( counter << 1 ) + 1 ] << 8 ) ;
				input <<= 16 ;
				input >>= 16 ;
				break ;
			case 4 :
				input = ucinputs[ counter << 2 ] + ( ucinputs[ ( counter << 2 ) + 1 ] << 8 )
						+ ( ucinputs[ ( counter << 2 ) + 2 ] << 16 )
						+ ( ucinputs[ ( counter << 2 ) + 3 ] << 24 ) ;
				break ;
			default :
				kfree( ( void * ) outputs ) ;
				return;
		}

		output = ConvRoutine( input ) ;

		switch( outBytes ) {
			case 1 :
				ucoutputs[ counter ] = ( unsigned char )output ;
				break ;
			case 2 :
				ucoutputs[ ( counter << 1 ) ] = ( unsigned char )output ;
				output >>= 8 ;
				ucoutputs[ ( counter << 1 ) + 1 ] = ( unsigned char )output ;
				break ;
			case 4 :
				ucoutputs[ ( counter << 2 ) ] = ( unsigned char )output ;
				output >>= 8 ;
				ucoutputs[ ( counter << 2 ) + 1 ] = ( unsigned char )output ;
				output >>= 8 ;
				ucoutputs[ ( counter << 2 ) + 2 ] = ( unsigned char )output ;
				output >>= 8 ;
				ucoutputs[ ( counter << 2 ) + 3 ] = ( unsigned char )output ;
				break ;
			default :
				break ;
		}
	}

	//return outputs ;
}
#endif
/*
void write_file_g711(INPARAM *inparam, unsigned int option )
{
	FILE      *oid;
	if( !inparam->output_buf ) {
		free( ( void * ) inparam->input_buf ) ;
		return;
	}

    switch( option ) {
    	case  1 :
    	case  2 :
    	case  3 :
    	case  4 :
    	case  5 :
    	case  6 :
    		oid = fopen( "test.711", "wb");
	      if (oid ==NULL){
	         printk("input file open fail.\n");
	         break;
	      }
    		fwrite (inparam->output_buf, 1, inparam->out_file_size, oid) ;
    		break ;
    	case  0 :
    	default :
    		break ;
    }

	free( ( void * ) inparam->input_buf ) ;
	free( ( void * ) inparam->output_buf ) ;

	return;
}
*/
