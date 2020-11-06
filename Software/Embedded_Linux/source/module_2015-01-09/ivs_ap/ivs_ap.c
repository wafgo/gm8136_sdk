#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <unistd.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <linux/version.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "ivs_ioctl.h"

#define IMEM_SIZE 		(4096*2160*2+1920*1080*4)
#define DEBUG_ENABLE	1
#define GOLD_COMPARE	1


/**************************************************************************
 *                                                                        
 *  Function Name:	 	
 *                                                                        
 *  Descriptions: 
 *                                                                        
 *  Arguments: 
 *                                                                        
 *  Returns: 
 *                                                                        
 *  See also: 
 *                                                                        
 **************************************************************************/
void usage(void)
{
	printf("Usage: ivs_ap -i1 input_image_1_filename -i2 input_image_2_filename [i2 only for sad] \n") ;
	printf("              -iw input_image_width      -ih input_image_height     [max 1920x1080][min 128*3] \n");
	printf("              -op [0: hsi_conver 1: de-interleave 2: integral image 3:SAD] \n");
	printf("              -bl [1: 1x1 3: 3x3 5: 5x5] SAD only \n");
	printf("              -sw [0: no_sw 1: sw_yc 2: sw_cbcr 3: sw_yc & cbcr] only for op 0, 1, 2 \n");
	printf("              -en [0: no_sw_end 1: sw_endianess] only for op 3 \n");
	printf("packed data format is supported in op 0, 1, 2 \n");
	printf("planar data format is supported in op 3 \n");
}
/**************************************************************************
 *                                                                        
 *  Function Name:	 	
 *                                                                        
 *  Descriptions: 
 *                                                                        
 *  Arguments: 
 *                                                                        
 *  Returns: 
 *                                                                        
 *  See also: 
 *                                                                        
 **************************************************************************/
int operationHsi( ivs_info_t ivs_info, 
                    ivs_param_t ivs_param,
                    unsigned char **H,
                    unsigned char **S,
                    unsigned char **I)
{
	int ret;
	ret = ioctl( ivs_info.fd, IVS_HSI_CONVERSION, &ivs_param);

	*H = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*2 ;
	*S = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*4 ;
	*I = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*5 ;
	return ret ;
}
/**************************************************************************
 *                                                                        
 *  Function Name:	 	
 *                                                                        
 *  Descriptions: 
 *                                                                        
 *  Arguments: 
 *                                                                        
 *  Returns: 
 *                                                                        
 *  See also: 
 *                                                                        
 **************************************************************************/
int operationDeInterleave( ivs_info_t ivs_info, 
                              ivs_param_t ivs_param,
	                          unsigned char **Y,
	                          unsigned char **CBCR)
{
	int ret;
	ret = ioctl( ivs_info.fd, IVS_DE_INTERLEAVE_YC, &ivs_param);	

	*Y    = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*2 ;
	*CBCR = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*3 ;
		
	return ret ;
}
/**************************************************************************
 *                                                                        
 *  Function Name:	 	
 *                                                                        
 *  Descriptions: 
 *                                                                        
 *  Arguments: 
 *                                                                        
 *  Returns: 
 *                                                                        
 *  See also: 
 *                                                                        
 **************************************************************************/
int operationIntegralImage( ivs_info_t ivs_info, 
                               ivs_param_t ivs_param,
	                           unsigned char **II)   
{
	int ret;
	ret = ioctl( ivs_info.fd, IVS_INTEGRAL_IMAGE, &ivs_param);

	*II = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*2 ;
	return ret;
}
/**************************************************************************
 *                                                                        
 *  Function Name:	 	
 *                                                                        
 *  Descriptions: 
 *                                                                        
 *  Arguments: 
 *                                                                        
 *  Returns: 
 *                                                                        
 *  See also: 
 *                                                                        
 **************************************************************************/
int operationSad( ivs_info_t ivs_info, 
                     ivs_param_t ivs_param,
	                 unsigned char **SAD)
{
	int ret;
	ret = ioctl( ivs_info.fd, IVS_SAD, &ivs_param);

	*SAD = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*2 ;
	return ret;
}
/**************************************************************************
 *                                                                        
 *  Function Name:	 	
 *                                                                        
 *  Descriptions: 
 *                                                                        
 *  Arguments: 
 *                                                                        
 *  Returns: 
 *                                                                        
 *  See also: 
 *                                                                        
 **************************************************************************/
int readImg( char *input_fn_1,
			   char *input_fn_2,	
	           ivs_info_t ivs_info, 
               ivs_param_t ivs_param)
{
	FILE *rd_chal_1, *rd_chal_2;
	int fret;

	if( ivs_param.operation!=3 )
	{
		rd_chal_1 = fopen(input_fn_1, "rb");
		if( rd_chal_1==NULL )
		{	
			printf("file is not exit \n");
			return -3;
		}
		
		fret = fread( ivs_info.mmap_addr, 
			          1, 
			          ivs_param.img_height*ivs_param.img_width*2, 
			          rd_chal_1);
		if( fret!= ivs_param.img_height*ivs_param.img_width*2 )
		{
			printf("file read 0x%x\n", fret);
			printf("image file error\n");
			return -3;
		}
		fclose(rd_chal_1);
	}
	else
	{
		rd_chal_1 = fopen(input_fn_1, "rb");
		if( rd_chal_1==NULL )
		{	
			printf("file is not exit \n");
			return -3;
		}
		
		fret = fread( ivs_info.mmap_addr, 
			          1, 
			          ivs_param.img_height*ivs_param.img_width, 
			          rd_chal_1);
		if( fret!= ivs_param.img_height*ivs_param.img_width )
		{
			printf("file read 0x%x\n", fret);
			printf("image file error\n");
			return -3;
		}
		fclose(rd_chal_1);

		rd_chal_2 = fopen(input_fn_2, "rb");
		if( rd_chal_2==NULL )
		{	
			printf("file is not exit \n");
			return -3;
		}
		
		fret = fread( ivs_info.mmap_addr+ivs_param.img_height*ivs_param.img_width, 
			          1, 
			          ivs_param.img_height*ivs_param.img_width, 
			          rd_chal_2);
		if( fret!= ivs_param.img_height*ivs_param.img_width )
		{
			printf("file read 0x%x\n", fret);
			printf("image file error\n");
			return -3;
		}
		fclose(rd_chal_2);
	}

	
	return 0;
}
/**************************************************************************
 *                                                                        
 *  Function Name:	 	
 *                                                                        
 *  Descriptions: 
 *                                                                        
 *  Arguments: 
 *                                                                        
 *  Returns: 
 *                                                                        
 *  See also: 
 *                                                                        
 **************************************************************************/
int driverInit(ivs_info_t *ivs_info)
{
	int ret=0;
	unsigned int API_version = IVS_VER;
	
	//	open node
	ivs_info->fd = open(IVS_DEV, O_RDWR);
	if (ivs_info->fd <= 0) {
    	printf("Fail to open %s\n", IVS_DEV);
 		return -2;   	
    }
	
	//	mmap
	ivs_info->mmap_addr = (int)mmap( 0, 
                           			 IMEM_SIZE, 
                           			 PROT_READ | PROT_WRITE, 
                           			 MAP_SHARED, 
                           			 ivs_info->fd, 
                           			 0);
	if (ivs_info->mmap_addr <= 0) {
		printf("Fail to use mmap, mmap_addr = %x!\n", ivs_info->mmap_addr);
		return -2;
	}
	
	//	check ap version
	ret = ioctl( ivs_info->fd, IVS_INIT, &API_version) ;	
	if( ret<0 )
		return -2;
	
	return 0;
}
/**************************************************************************
 *                                                                        
 *  Function Name:	 	
 *                                                                        
 *  Descriptions: 
 *                                                                        
 *  Arguments: 
 *                                                                        
 *  Returns: 
 *                                                                        
 *  See also: 
 *                                                                        
 **************************************************************************/
void driverClose(ivs_info_t ivs_info)
{
	if( ivs_info.fd ) 
	{
		munmap( (void*)ivs_info.mmap_addr, 
			    IMEM_SIZE);
		close(ivs_info.fd);
	}
}
/**************************************************************************
 *                                                                        
 *  Function Name:	 	
 *                                                                        
 *  Descriptions: 
 *                                                                        
 *  Arguments: 
 *                                                                        
 *  Returns:   		 0 : succesee
 *            	   	-1 : command line parameter error
 *                 	-2 : driver initial error
 *            		-3 : read input image error
 *            		-4 : copy from user error
 *                  -5 : software time out 
 *                  -6 : unknown ioctl
 *
 *  See also: 
 *                                                                        
 **************************************************************************/
int main(int argc, char ** argv)
{
	//		function return value
	int ret=0;

	//	1.	receive command line parameters 
	int i=0;
	
	char *input_fn_1=NULL,  *input_fn_2=NULL;	
	int   img_width=0,  img_height=0;
	char  operation=0,   blk_size=0,   swap=0,   endian=0;	

	ivs_param_t ivs_param;

	//	2.	initial_driver parameter
	ivs_info_t ivs_info;
	
	//	3.	operation parameter 
	unsigned char *H=NULL, *S=NULL, *I=NULL;
	unsigned char *Y=NULL, *CBCR=NULL;
	unsigned char *II=NULL;
	unsigned char *SAD=NULL;
	
#if(DEBUG_ENABLE==1)
	FILE *debug = fopen("debug.txt","w") ;
	FILE *wt1=NULL, *wt2=NULL, *wt3=NULL;
	char FN[20];
	char c_num[20];
#endif

	//	1. receive command line 
	i = 1;
	while (i < argc) {
		if( (strcmp(argv[i], "-i1") == 0) && (++i < argc) )
			input_fn_1 = argv[i];
		else if( (strcmp(argv[i], "-i2") == 0) && (++i < argc) )
			input_fn_2 = argv[i];
		else if( (strcmp(argv[i], "-iw") == 0) && (++i < argc) )
			img_width = atoi(argv[i]);
		else if( (strcmp(argv[i], "-ih") == 0) && (++i < argc) )
			img_height = atoi(argv[i]);
		else if( (strcmp(argv[i], "-op") == 0) && (++i < argc) )
			operation = atoi(argv[i]);
		else if( (strcmp(argv[i], "-bl") == 0) && (++i < argc) )
			blk_size = atoi(argv[i]);		
		else if( (strcmp(argv[i], "-sw") == 0) && (++i < argc) )
			swap = atoi(argv[i]);
		else if( (strcmp(argv[i], "-en") == 0) && (++i < argc) )
			endian = atoi(argv[+i]);
		else {
		  	printf ("unknow parameter %s\n", argv[i]);
			usage();
			return -1;
		}
		i++ ;
	}
		
	if ( input_fn_1 == NULL ) {
		printf("Pls specify input file\n");
		return -1;
	}

	if ( input_fn_2 == NULL && operation==3) {
		printf("SAD need two input\n");
		return -1;
	}
		
	if ( img_width > 1920 || img_height > 1080 || img_height < 3 || (img_width%128)!=0 ) {
		printf("error image size\n");
		return -1;
	}
			
	if ( operation<0 || operation>3 || swap<0 || swap>3 || endian<0 || endian>1) {
		printf("parameter incorrect\n");
		return -1;
	}

	if ( operation==3 && swap!=0 ) {
		printf("swap_cbcr & yc not support SAD operation\n");
		return -1;
	}

	if ( operation!=3 && endian!=0 ) {
		printf("sw_endianess not support HSI conversion, De-interleave YC and Integral image operation\n");
		return -1;
	}
	
	ivs_param.img_height = img_height ;    
	ivs_param.img_width  = img_width  ;
	ivs_param.operation  = operation  ;
	ivs_param.blk_size   = blk_size   ;
	ivs_param.swap       = swap       ;
	ivs_param.endian     = endian     ;

	//	2. initial_driver
	ret = driverInit( &ivs_info );
	if(ret<0)
		return ret;

	//	3. open & read input image
	ret = readImg( input_fn_1,
	               input_fn_2,
	               ivs_info,
	               ivs_param ) ;

	if( ret<0 )
		goto error;

#if(DEBUG_ENABLE== 1)
    printf("Height=%d, Width=%d, Operation=%d\n",ivs_param.img_height,
		                                         ivs_param.img_width,
		                                         ivs_param.operation);
#endif	
	//	4. operation
	if( operation==0 ) {
		ret = operationHsi( ivs_info, 
			                ivs_param,
			                &H,
			                &S,
			                &I);

#if(DEBUG_ENABLE== 1)
		if( ret==-4 )
			fprintf(debug,"hsi conversion copy from user error %d %d\n",img_height,img_width) ;
		else if( ret==-5 )
	 		fprintf(debug,"hsi conversion software time out    %d %d\n",img_height,img_width) ;
		else
		{
#if(GOLD_COMPARE==1)
			strcpy(FN,"t_h_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wt1 = fopen(FN,"wb");
			
			strcpy(FN,"t_s_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wt2 = fopen(FN,"wb");

			strcpy(FN,"t_i_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wt3 = fopen(FN,"wb") ;
			
			fwrite(H,1,img_height*img_width*2,wt1) ;
			fwrite(S,1,img_height*img_width,wt2) ;
			fwrite(I,1,img_height*img_width,wt3) ;

			fclose(wt1) ;
			fclose(wt2) ;
			fclose(wt3) ;
#endif
        }
#endif
		if( ret<0 )
			goto error;
	} 
	else if( operation==1) 
	{
		ret = operationDeInterleave( ivs_info, 
			                         ivs_param,
			                         &Y,
			                         &CBCR);
#if(DEBUG_ENABLE==1)
		if( ret==2 )
			fprintf(debug,"de interleave  copy from user error %d %d\n",img_height,img_width) ;
		else if( ret==3 )
	 		fprintf(debug,"de interleave  software time out    %d %d\n",img_height,img_width) ;
		else		
		{
#if(GOLD_COMPARE==1)			
			strcpy(FN,"t_y_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wt1 = fopen(FN,"wb") ;

			strcpy(FN,"t_cbcr_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wt2 = fopen(FN,"wb") ;

			printf("Write De-interleave Result\n");
			
			fwrite(Y   ,1,img_height*img_width,wt1) ;
			fwrite(CBCR,1,img_height*img_width,wt2) ;

			fclose(wt1) ;
			fclose(wt2) ;	
#endif
        }
#endif
		if( ret<0 )
			goto error;
	} 
	else if( operation==2 ) 
	{
		ret = operationIntegralImage( ivs_info,
			                          ivs_param,
			                          &II);
#if(DEBUG_ENABLE==1)			
		if( ret==2 )
			fprintf(debug,"integral image copy from user error %d %d\n",img_height,img_width) ;
		else if( ret==3 )
	 		fprintf(debug,"integral image software time out    %d %d\n",img_height,img_width) ;
		else
		{
#if(GOLD_COMPARE==1)
			strcpy(FN,"t_ii_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wt1 = fopen(FN,"wb") ;

			printf("Write Integral Image Result\n");

			fwrite(II,1,img_height*img_width*4,wt1) ;

			fclose(wt1) ;	
#endif
	    }
#endif
		if( ret<0 )
			goto error;
	
	}
	else 
	{
		ret = operationSad( ivs_info,
			                ivs_param,
			                &SAD);
#if(DEBUG_ENABLE==1)			
		if( ret==2 )
			fprintf(debug,"sad copy from user error %d %d\n",img_height,img_width) ;
		else if( ret==3 )
	 		fprintf(debug,"sad software time out    %d %d\n",img_height,img_width) ;
		else
		{
#if(GOLD_COMPARE==1)
			strcpy(FN,"t_sad_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wt1 = fopen(FN,"wb") ;

			printf("Write SAD Result\n");

			fwrite(SAD,1,img_height*img_width*2,wt1) ;

			fclose(wt1) ;
#endif
	    }
#endif
		if( ret<0 )
			goto error;
	}

	
error:
	driverClose(ivs_info);

#if(DEBUG_ENABLE)
	fclose(debug) ;
#endif

	return ret;
}

