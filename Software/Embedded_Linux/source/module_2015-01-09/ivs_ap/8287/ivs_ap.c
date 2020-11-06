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

#define IMEM_SIZE 		(1920 * 1080 * 8)
#define DEBUG_ENABLE	1

static int sobel[25] = {
   1,  4,  6,  4,  1,
   2,  8, 12,  8,  2,
   0,  0,  0,  0,  0,
  -2, -8,-12, -8, -2,
  -1, -4, -6, -4, -1   };

void usage(void)
{   
	printf("hsi \n");
	printf("ivs_ap -op 0 -i1 -iw -ih -sw \n");

	printf("rgb \n");
	printf("ivs_ap -op 1 -i1 -iw -ih -sw \n");

	printf("integral image \n");
	printf("ivs_ap -op 2 -i1 -iw -ih -if -sw -en\n");

	printf("squared integral image \n");
	printf("ivs_ap -op 3 -i1 -iw -ih -if -sw -en\n");

	printf("de-interleave YC\n");
	printf("ivs_ap -op 4 -i1 -iw -ih -sw \n");

	printf("histogram\n");
	printf("ivs_ap -op 5 -i1 -iw -ih -if -sw -en\n");

	printf("convolution\n");
	printf("ivs_ap -op 6 -i1 -iw -ih -if -sh -bth -en\n");

	printf("morphology\n")	;
	printf("ivs_ap -op 7 -i1 -iw -ih -if -mp -sto -mth\n");

	printf("sad\n")	;
	printf("ivs_ap -op 8 -i1 -i2 -iw -ih -bl -bth -en \n");

	printf("roster\n")	;
	printf("ivs_ap -op 9 -i1 -i2 -iw -ih -rop -en \n");
}

int check_parameter( ivs_param_t ivs_param,
                     char *input_fn_2 )
{
    if ((ivs_param.operation > 7) && (input_fn_2 ==NULL)) {
        printf("Need two input files\n");
        return -1;
    }

    if ((ivs_param.file_format == 0) && (ivs_param.endian == 1)) {
        printf("packed data format not support swap endian\n");
        return -1;
    }

    if ((ivs_param.file_format == 1) && (ivs_param.swap == 1)) {
        printf("planar data format not support swap yc\n");
        return -1;
    }

    if ((ivs_param.file_format == 2) && ((ivs_param.endian == 1) || (ivs_param.swap == 1))) {
        printf("binary data format not support swap yc & endian\n");
        return -1;
    }

    if ((ivs_param.operation == 0) && (ivs_param.file_format != 0)) {
        printf("file format error for op 0\n");
        return -1;
    }

    if ((ivs_param.operation == 1) && (ivs_param.file_format != 0)) {
        printf("file format error for op 1\n");
        return -1;
    }

    if ((ivs_param.operation == 2) && (ivs_param.file_format == 2)) {
        printf("file format error for op 2\n");
        return -1;
    }

    if ((ivs_param.operation == 3) && (ivs_param.file_format == 2)) {
        printf("file format error for op 3\n");
        return -1;
    }

    if ((ivs_param.operation == 4) && (ivs_param.file_format != 0)) {
        printf("file format error for op 4\n");
        return -1;
    }

    
    
    return 0;
}

int operationHsi( ivs_info_t ivs_info, 
                  ivs_param_t ivs_param,
                  unsigned char **H,
                  unsigned char **S,
                  unsigned char **I)
{
	int ret;
	ret = ioctl( ivs_info.fd, IVS_HSI_CONVERSION, &ivs_param);

	*H = ivs_info.mmap_addr + ivs_param.img_height * ivs_param.img_width * 2;
	*S = ivs_info.mmap_addr + ivs_param.img_height * ivs_param.img_width * 4;
	*I = ivs_info.mmap_addr + ivs_param.img_height * ivs_param.img_width * 5;
	return ret ;
}

int operationRgb( ivs_info_t ivs_info, 
                  ivs_param_t ivs_param,
                  unsigned char **R,
                  unsigned char **G,
                  unsigned char **B)
{
	int ret;
	ret = ioctl( ivs_info.fd, IVS_RGB_CONVERSION, &ivs_param);

	*R = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*2 ;
	*G = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*3 ;
	*B = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*4 ;
	return ret ;
}

int operationIntegralImage( ivs_info_t ivs_info, 
                            ivs_param_t ivs_param,
	                        unsigned char **II)   
{
	int ret;
	ret = ioctl( ivs_info.fd, IVS_INTEGRAL_IMAGE, &ivs_param);

	*II = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*2 ;
	return ret;
}

int operationSquaredIntegralImage( ivs_info_t ivs_info, 
                                   ivs_param_t ivs_param,
	                               unsigned char **BII)   
{
	int ret;
	ret = ioctl( ivs_info.fd, IVS_SQUARED_INTEGRAL_IMAGE, &ivs_param);

	*BII = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*2 ;
	return ret;
}

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

int operationHistogram( ivs_info_t ivs_info, 
                        ivs_param_t ivs_param,
	                    unsigned char **HI)   
{
	int ret;
	ret = ioctl( ivs_info.fd, IVS_HISTOGRAM, &ivs_param);

	*HI = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*2 ;
	return ret;
}

int operationConvolution( ivs_info_t ivs_info, 
                          ivs_param_t ivs_param,
                          unsigned char **H,
                          unsigned char **S,
                          unsigned char **I)
{
	int ret;
	ret = ioctl( ivs_info.fd, IVS_CONVOLUTION, &ivs_param);

	*H = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*2 ;
	*S = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*4 ;
	*I = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*6 ;
	return ret ;
}

int operationMorphology( ivs_info_t ivs_info, 
                         ivs_param_t ivs_param,
                         unsigned char **H)
{
    int ret;
	ret = ioctl( ivs_info.fd, IVS_MORPHOLOGY, &ivs_param);

	*H = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*2 ;
	return ret ;
}

int operationSad( ivs_info_t ivs_info, 
                  ivs_param_t ivs_param,
	              unsigned char **SAD)
{
	int ret;
	ret = ioctl( ivs_info.fd, IVS_SAD, &ivs_param);

	*SAD = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*2 ;
	return ret;
}

int operationRoster( ivs_info_t ivs_info, 
                     ivs_param_t ivs_param,
	                 unsigned char **ROS)
{
	int ret;
	ret = ioctl( ivs_info.fd, IVS_RASTER_OPERATION, &ivs_param);

	*ROS = ivs_info.mmap_addr + ivs_param.img_height*ivs_param.img_width*2 ;
	return ret;
}

int readImg( char *input_fn, 
             ivs_info_t ivs_info, 
             ivs_param_t ivs_param,
             int num)
{
	int img_width   = ivs_param.img_width;
	int img_height  = ivs_param.img_height;
	int file_format = ivs_param.file_format;
	int data_size   = 0;
	
	FILE *rd_chal;
	int fret;

	if (input_fn != NULL) 
	{
		rd_chal = fopen(input_fn, "rb");
		if (rd_chal == NULL)
		{	
			printf("file is not exit \n");
			return -3;
		}

		if (file_format == 0)
			data_size = img_width * img_height * 2;
		else if (file_format == 1)
			data_size = img_width * img_height;
		else if (file_format == 2)
			data_size = (img_width * img_height) / 8;

        if (num == 1)
        {
            fret = fread( ivs_info.mmap_addr, 
			              1, 
			              data_size, 
			              rd_chal);
		
	    	if (fret != data_size)
		    {
			    printf("file read 0x%x\n", fret);
			    printf("image file error\n");
			    return -3;
		    }
		    fclose(rd_chal);
        } 
        else 
        {
            fret = fread( ivs_info.mmap_addr + data_size, 
			              1, 
			              data_size, 
			              rd_chal);
		
	    	if (fret != data_size)
		    {
			    printf("file read 0x%x\n", fret);
			    printf("image file error\n");
			    return -3;
		    }
		    fclose(rd_chal);    
        }
	}

	return 0;
}

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
    int i = 0 ;
    int ret = 0 ;

	//  receive command line parameters 
	char *input_fn_1           = NULL ;  
	char *input_fn_2           = NULL ;	
	int img_width              = 0 ;
	int img_height             = 0 ;
	int operation              = 0 ;
    int sel_thres_output       = 0 ;
    int threshold              = 0 ;
    int raster_op              = 0 ;
	int file_format            = 0 ;
	int morphology_operation   = 0 ;	  
	int block_size             = 0 ;
	int swap                   = 0 ;	
	int endian                 = 0 ;
	int kernel_template_nunber = 0 ;
	int convolution_shift      = 0 ;

    //  ivs_driver parameter
	ivs_param_t ivs_param;
	ivs_info_t ivs_info;
	
	//  output data address 
	unsigned char *WCH_1=NULL, *WCH_2=NULL, *WCH_3=NULL;

#if (DEBUG_ENABLE == 1)
	FILE *debug = fopen("debug.txt","w") ;
	FILE *wch_1=NULL, *wch_2=NULL, *wch_3=NULL;
	char FN[20];
	char c_num[20];
#endif

	//  receive command line 
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
		else if( (strcmp(argv[i], "-sto") == 0) && (++i < argc) )
			sel_thres_output = atoi(argv[i]);
		else if( (strcmp(argv[i], "-th") == 0) && (++i < argc) )
			threshold = atoi(argv[i]);
		else if( (strcmp(argv[i], "-rop") == 0) && (++i < argc) )
			raster_op = atoi(argv[i]);
		else if( (strcmp(argv[i], "-if") == 0) && (++i < argc) )
			file_format = atoi(argv[i]);
		else if( (strcmp(argv[i], "-mp") == 0) && (++i < argc) )
			morphology_operation = atoi(argv[i]);
		else if( (strcmp(argv[i], "-bl") == 0) && (++i < argc) )
			block_size = atoi(argv[i]);
		else if( (strcmp(argv[i], "-sw") == 0) && (++i < argc) )
			swap = atoi(argv[i]);
		else if( (strcmp(argv[i], "-en") == 0) && (++i < argc) )
			endian = atoi(argv[i]);
		else if( (strcmp(argv[i], "-ktn") == 0) && (++i < argc) )
			kernel_template_nunber = atoi(argv[i]);
		else if( (strcmp(argv[i], "-sh") == 0) && (++i < argc) )
			convolution_shift = atoi(argv[i]);
		else {
		  	printf ("unknow parameter %s\n", argv[i]);
			usage();
			return -1;
		}
		i++ ;
	}
			
	ivs_param.img_width				 = img_width;
	ivs_param.img_height			 = img_height;
	ivs_param.operation				 = operation;
	ivs_param.sel_thres_output		 = sel_thres_output;
	ivs_param.threshold	             = threshold;  
	ivs_param.raster_op				 = raster_op;
	ivs_param.file_format 			 = file_format;
	ivs_param.morphology_operation   = morphology_operation;	  
	ivs_param.block_size			 = block_size;
	ivs_param.swap					 = swap;	
	ivs_param.endian				 = endian;
	ivs_param.kernel_template_nunber = kernel_template_nunber;
	ivs_param.convolution_shift		 = convolution_shift;
	for (i = 0; i < 25; i++)
		ivs_param.template_element[i] =	sobel[i];

    //  check parameter
    ret = check_parameter(ivs_param,input_fn_2);
    if (ret < 0)
        return ret;

	//  initial_driver
	ret = driverInit(&ivs_info);
	if (ret < 0)
	    return ret;

	//  open & read input image
	ret = readImg( input_fn_1, ivs_info, ivs_param, 1);
	if (ret < 0)
		goto error;

	ret = readImg( input_fn_2, ivs_info, ivs_param, 2);
	if (ret < 0)
		goto error;

#if(DEBUG_ENABLE== 1)
    printf("Height=%d, Width=%d, Operation=%d\n",ivs_param.img_height,
		                                         ivs_param.img_width,
		                                         ivs_param.operation);
#endif	
	//	4. operation
	if (operation == 0) {
		ret = operationHsi( ivs_info, 
			                ivs_param,
			                &WCH_1,
			                &WCH_2,
			                &WCH_3);

#if (DEBUG_ENABLE == 1)
		if (ret == -4)
		    fprintf(debug,"hsi conversion copy from user error %d %d\n",img_height,img_width) ;
		else if (ret == -5)
	 		fprintf(debug,"hsi conversion software time out %d %d\n",img_height,img_width) ;
		else
		{
			strcpy(FN,"t_h_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wch_1 = fopen(FN,"wb");
			
			strcpy(FN,"t_s_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wch_2 = fopen(FN,"wb");

			strcpy(FN,"t_i_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wch_3 = fopen(FN,"wb") ;
            
			printf("Write HSI Result\n");
            
			fwrite(WCH_1,1,img_height*img_width*2,wch_1) ;
			fwrite(WCH_2,1,img_height*img_width,wch_2) ;
			fwrite(WCH_3,1,img_height*img_width,wch_3) ;

			fclose(wch_1) ;
			fclose(wch_2) ;
			fclose(wch_3) ;
        }
#endif
		if (ret < 0)
			goto error;
	} 
    else if (operation == 1) {
        ret = operationRgb( ivs_info, 
			                ivs_param,
			                &WCH_1,
			                &WCH_2,
			                &WCH_3);

#if (DEBUG_ENABLE == 1)
		if (ret == -4)
			fprintf(debug,"rgb conversion copy from user error %d %d\n",img_height,img_width) ;
		else if (ret == -5)
	 		fprintf(debug,"rgb conversion software time out %d %d\n",img_height,img_width) ;
		else
		{
			strcpy(FN,"t_r_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wch_1 = fopen(FN,"wb");
			
			strcpy(FN,"t_g_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wch_2 = fopen(FN,"wb");

			strcpy(FN,"t_b_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wch_3 = fopen(FN,"wb") ;

            printf("Write RGB Result\n");
            
			fwrite(WCH_1,1,img_height*img_width,wch_1) ;
			fwrite(WCH_2,1,img_height*img_width,wch_2) ;
			fwrite(WCH_3,1,img_height*img_width,wch_3) ;

			fclose(wch_1) ;
			fclose(wch_2) ;
			fclose(wch_3) ;
        }
#endif
		if (ret < 0)
			goto error;
	} 
    else if( operation == 2 ) 
	{
		ret = operationIntegralImage( ivs_info,
			                          ivs_param,
			                          &WCH_1);
#if(DEBUG_ENABLE==1)			
		if( ret==2 )
			fprintf(debug,"integral image copy from user error %d %d\n",img_height,img_width) ;
		else if( ret==3 )
	 		fprintf(debug,"integral image software time out %d %d\n",img_height,img_width) ;
		else
		{
			strcpy(FN,"t_ii_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wch_1 = fopen(FN,"wb") ;

			printf("Write Integral Image Result\n");

			fwrite(WCH_1,1,img_height * img_width * 4,wch_1) ;

			fclose(wch_1) ;	
	    }
#endif
		if( ret<0 )
			goto error;
	
	}
    else if( operation == 3 ) 
	{
		ret = operationSquaredIntegralImage( ivs_info,
			                                 ivs_param,
			                                 &WCH_1);
#if(DEBUG_ENABLE==1)			
		if( ret==2 )
			fprintf(debug,"squared integral image copy from user error %d %d\n",img_height,img_width) ;
		else if( ret==3 )
	 		fprintf(debug,"squared integral image software time out %d %d\n",img_height,img_width) ;
		else
		{
			strcpy(FN,"t_sii_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wch_1 = fopen(FN,"wb") ;

			printf("Write squared integral image Result\n");

			fwrite(WCH_1,1,img_height * img_width * 4,wch_1) ;

			fclose(wch_1) ;	
	    }
#endif
		if( ret<0 )
			goto error;
	
	}
    else if(operation == 4) 
	{
		ret = operationDeInterleave( ivs_info, 
			                         ivs_param,
			                         &WCH_1,
			                         &WCH_2);
#if(DEBUG_ENABLE==1)
		if( ret==2 )
			fprintf(debug,"de interleave copy from user error %d %d\n",img_height,img_width) ;
		else if( ret==3 )
	 		fprintf(debug,"de interleave software time out %d %d\n",img_height,img_width) ;
		else		
		{		
			strcpy(FN,"t_y_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wch_1 = fopen(FN,"wb") ;

			strcpy(FN,"t_cbcr_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wch_2 = fopen(FN,"wb") ;

			printf("Write De-interleave Result\n");
			
			fwrite(WCH_1,1,img_height * img_width,wch_1) ;
			fwrite(WCH_2,1,img_height * img_width,wch_2) ;

			fclose(wch_1) ;
			fclose(wch_2) ;	
        }
#endif
		if( ret<0 )
			goto error;
	} 
    else if(operation == 5) 
	{
		ret = operationHistogram( ivs_info, 
			                      ivs_param,
			                      &WCH_1);
#if(DEBUG_ENABLE==1)
		if( ret==2 )
			fprintf(debug,"histogram copy from user error %d %d\n",img_height,img_width) ;
		else if( ret==3 )
	 		fprintf(debug,"histogram software time out    %d %d\n",img_height,img_width) ;
		else		
		{		
			strcpy(FN,"t_hi_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wch_1 = fopen(FN,"wb"); 
                
			printf("Write Histogram Result\n");
			
			fwrite(WCH_1,1,1024,wch_1) ;

			fclose(wch_1) ;
        }
#endif
		if( ret<0 )
			goto error;
	} 
    else if(operation == 6) 
	{
		ret = operationConvolution( ivs_info, 
			                        ivs_param,
			                        &WCH_1,
			                        &WCH_2,
			                        &WCH_3);
#if(DEBUG_ENABLE==1)
		if( ret==2 )
			fprintf(debug,"convolution copy from user error %d %d\n",img_height,img_width) ;
		else if( ret==3 )
	 		fprintf(debug,"convolution software time out    %d %d\n",img_height,img_width) ;
		else		
		{		
			strcpy(FN,"t_h_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wch_1 = fopen(FN,"wb"); 

            strcpy(FN,"t_s_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wch_2 = fopen(FN,"wb"); 

            strcpy(FN,"t_i_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wch_3 = fopen(FN,"wb"); 
                
			printf("Write convolution Result\n");

            if (convolution_shift==0)
			    fwrite(WCH_1,1,img_height*img_width*2,wch_1) ;
            else
                fwrite(WCH_1,1,img_height*img_width,wch_1) ;
            fwrite(WCH_2,1,img_height*img_width*2,wch_2) ;
            fwrite(WCH_3,1,img_height*img_width*2,wch_3) ;

			fclose(wch_1) ;
            fclose(wch_2) ;
            fclose(wch_3) ;
        }
#endif
		if( ret<0 )
			goto error;
	} 
    else if(operation == 7) 
	{
		ret = operationMorphology( ivs_info, 
			                       ivs_param,
			                       &WCH_1);
#if(DEBUG_ENABLE==1)
		if( ret==2 )
			fprintf(debug,"morphology copy from user error %d %d\n",img_height,img_width) ;
		else if( ret==3 )
	 		fprintf(debug,"morphology software time out    %d %d\n",img_height,img_width) ;
		else		
		{		
			strcpy(FN,"t_mor_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wch_1 = fopen(FN,"wb"); 
    
			printf("Write morphology Result\n");

            if (sel_thres_output == 0)
			    fwrite(WCH_1,1,img_height*img_width/8,wch_1) ;
            else
                fwrite(WCH_1,1,img_height*img_width,wch_1) ;

			fclose(wch_1) ;
        }
#endif
		if( ret<0 )
			goto error;
	} 
	else if( operation == 8 )
	{
		ret = operationSad( ivs_info,
			                ivs_param,
			                &WCH_1);
#if(DEBUG_ENABLE==1)			
		if( ret==2 )
			fprintf(debug,"sad copy from user error %d %d\n",img_height,img_width) ;
		else if( ret==3 )
	 		fprintf(debug,"sad software time out %d %d\n",img_height,img_width) ;
		else
		{
			strcpy(FN,"t_sad_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wch_1 = fopen(FN,"wb") ;

			printf("Write SAD Result\n");

            if (threshold == 0)
			    fwrite(WCH_1,1,img_height*img_width*2,wch_1) ;
            else
                fwrite(WCH_1,1,img_height*img_width/8,wch_1) ;
			fclose(wch_1) ;
	    }
#endif
		if( ret<0 )
			goto error;
	}
    else if (operation == 9)
	{
		ret = operationRoster (ivs_info,
			                   ivs_param,
			                   &WCH_1);
#if(DEBUG_ENABLE==1)			
		if( ret==2 )
			fprintf(debug,"roster copy from user error %d %d\n",img_height,img_width);
		else if( ret==3 )
	 		fprintf(debug,"roster software time out %d %d\n",img_height,img_width);
		else
		{
			strcpy(FN,"t_rast_");
			sprintf(c_num,"%d",img_height);
			strcat(FN,c_num);
			strcat(FN,"_");
			sprintf(c_num,"%d",img_width);
			strcat(FN,c_num);
			wch_1 = fopen(FN,"wb") ;

			printf("Write raster Result\n");

            if (file_format == 1)
			    fwrite(WCH_1,1,img_height*img_width,wch_1) ;
            else
                fwrite(WCH_1,1,img_height*img_width/8,wch_1) ;
			fclose(wch_1) ;
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

