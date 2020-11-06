#include <linux/string.h>
#include <linux/slab.h>
#include "g711s.h"

int g711_init_enc(AUDIO_CODEC_SETTING *codec_setting, INPARAM *inparam[])
{
	int i;
	unsigned short block_size;
	int MultiChannel;
	unsigned short aulaw_enc_option;

	block_size = codec_setting->aulaw_codec_pars.block_size;
	aulaw_enc_option = codec_setting->aulaw_codec_pars.aulaw_enc_option;
	MultiChannel = codec_setting->EncMultiChannel;

	if(!codec_setting)
	   return 1;

	//codec_setting->InputSampleNumber = 505;
	for(i=0;i<MultiChannel;i++)
	{
            	inparam[i] = (INPARAM *)kmalloc(sizeof(INPARAM), GFP_KERNEL);
     		if(!inparam[i])
     		{
         		printk("init fail to create inparam_enc struct\n");
         		return 1;
     		}
		inparam[i]->block_size = block_size;
		inparam[i]->option = aulaw_enc_option;
    		inparam[i]->input_buf = (char *)kmalloc (inparam[i]->block_size*sizeof(short), GFP_KERNEL);
     		if(!inparam[i]->input_buf)
     		{
         		printk("init fail to create encoder input buffer\n");
         		return 1;
      		}

     		inparam[i]->output_buf  = (char *)kmalloc (inparam[i]->block_size, GFP_KERNEL);
     		if(!inparam[i]->output_buf)
     		{
         		printk("init fail to create encoder ouput buffer\n");
         		return 1;
      		}
    	 }

	return 0;
}
EXPORT_SYMBOL(g711_init_enc);

int g711_init_dec(AUDIO_CODEC_SETTING *codec_setting, INPARAM **inparam1)
{
	unsigned short block_size;
	unsigned short aulaw_dec_option;
	INPARAM *inparam;

	block_size = codec_setting->aulaw_codec_pars.block_size;
	aulaw_dec_option = codec_setting->aulaw_codec_pars.aulaw_dec_option;

	if(!codec_setting)
	   return 1;

      	inparam = (INPARAM *)kmalloc(sizeof(INPARAM), GFP_KERNEL);
     	       if(!inparam)
     		{
         		printk("init fail to create inparam_dec struct\n");
         		return 1;
     		}
		inparam->block_size = block_size;
		inparam->option = aulaw_dec_option;
    		inparam->input_buf = (char *)kmalloc (inparam->block_size, GFP_KERNEL);
     		if(!inparam->input_buf)
     		{
         		printk("init fail to create decoder input buffer\n");
         		return 1;
      		}

     		inparam->output_buf  = (char *)kmalloc (inparam->block_size*sizeof(short), GFP_KERNEL);
     		if(!inparam->output_buf)
     		{
         		printk("init fail to create decoder ouput buffer\n");
         		return 1;
      		}
	*inparam1 = inparam;
	return 0;
}
EXPORT_SYMBOL(g711_init_dec);


void g711enc_destory(INPARAM *inparam[], short MultiChannel)
{
    int i;
	for(i=0;i<MultiChannel;i++)
	{
		if( inparam[i]->input_buf)
		{
	    		kfree( ( char * ) inparam[i]->input_buf ) ;
			inparam[i]->input_buf	= NULL;
		}
		if( inparam[i]->output_buf)
		{
	    		kfree( ( char * ) inparam[i]->output_buf ) ;
			inparam[i]->output_buf = NULL;
		}
		if(inparam[i])
		{
	    		kfree(inparam[i]);
			inparam[i] = NULL;
		}
	}
}
EXPORT_SYMBOL(g711enc_destory);

void g711dec_destory(INPARAM *inparam)
{
		if( inparam->input_buf)
		{
	    		kfree( ( char * ) inparam->input_buf ) ;
			inparam->input_buf	= NULL;
		}
		if( inparam->output_buf)
		{
	    		kfree( ( char * ) inparam->output_buf ) ;
			inparam->output_buf = NULL;
		}
		if(inparam)
		{
	    		kfree(inparam);
			inparam = NULL;
		}
}
EXPORT_SYMBOL(g711dec_destory);
