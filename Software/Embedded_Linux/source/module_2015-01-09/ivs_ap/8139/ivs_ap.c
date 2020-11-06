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

#define IMEM_SIZE 				(1080 * 1920 * 10)
#define GOLDEN_COMPARE			1
#define max_width 				1920
#define max_height 				1080

static int sobel[25] = {
   1,  4,  6,  4,  1,
   2,  8, 12,  8,  2,
   0,  0,  0,  0,  0,
  -2, -8,-12, -8, -2,
  -1, -4, -6, -4, -1   };

static int morph[25] = {
   0,  0,  0,  0,  0,
   1,  1,  1,  1,  1,
   0,  0,  0,  0,  0,
   1,  1,  1,  1,  1,
   0,  0,  0,  0,  0   };

void usage(void)
{   
    printf("-i1: input file 1, -i2: input file 2, -iw: image width, -ih: image height,      \n");
    printf("-if: file format (4: ii or sii, 2: packed, 1: planar, 0: binary),               \n");
    printf("-op: operation index (0: HSI, 1: RGB, 2: II, 3: SII, 4: de-interleaving,        \n");
    printf("                      5: histogram, 6: convolution, 7: morphology               \n");
    printf("                      8: SAD, 9: raster, 10: cascade),                          \n");
    printf("-kt: kernel template index, -sh: shifted convolution sum -th: threshold,        \n");
    printf("-mp: morphology operator    -st: select_threshold_output                        \n");
    printf("-bl: block size,            -ro: raster operationo code  -ic: integral cascade, \n"); 
    printf("-sw: swap Y CbCr,           -en: swap endian                                  \n\n");

    printf(" HSI            : ivs_ap -i1     -iw -ih -if -op 0                  -sw     \n ");
    printf(" RGB            : ivs_ap -i1     -iw -ih -if -op 1                  -sw     \n ");
    printf(" II             : ivs_ap -i1     -iw -ih -if -op 2                  -sw -en \n ");
    printf(" SII            : ivs_ap -i1     -iw -ih -if -op 3                  -sw -en \n ");
    printf(" de-interleaving: ivs_ap -i1     -iw -ih -if -op 4                  -sw     \n ");
    printf(" histogram      : ivs_ap -i1     -iw -ih -if -op 5                  -sw -en \n ");
    printf(" convolution    : ivs_ap -i1     -iw -ih -if -op 6  -sh     -th -kt     -en \n ");
    printf(" morphology     : ivs_ap -i1     -iw -ih -if -op 7  -mp -st -th -kt     -en \n ");
    printf(" SAD            : ivs_ap -i1 -i2 -iw -ih -if -op 8  -bl     -th         -en \n ");
    printf(" raster         : ivs_ap -i1 -i2 -iw -ih -if -op 9  -ro                 -en \n ");
    printf(" cascade        : ivs_ap -i1 -i2 -iw -ih -if -op 10 -ic             -sw -en \n ");
}

int check_parameter(ivs_ioctl_param_t ivs_param,
                    char *input_fn_1, char *input_fn_2)
{
    int ff = ivs_param.img_info.img_format;
    int sw = ivs_param.img_info.swap_y_cbcr;
    int en = ivs_param.img_info.swap_endian;
    int op = ivs_param.operation_info.operation;
    int ic = ivs_param.operation_info.integral_cascade;

    if (input_fn_1 == NULL) {
        printf("input image 1 is NULL \n");
        return -1;
    }

    if ((op == 8) || (op == 9) || ((op == 10) && (ic == 0)))
        if (input_fn_2 == NULL) {
            printf("input image 2 is NULL \n");
            return -1;
        }

    if ((ff == 2) && (en != 0)) {
        printf("swap endian not support packed format\n");
        return -1;
    }

    if ((ff != 2) && (sw != 0)) {
        printf("swap y cbcr not support planar format\n");
        return -1;
    }

    if ((op < 0) || (op > 10)) {
        printf("no support operation\n");
        return -1;
    }

    return 0;
}

int operationHsi(ivs_info_t ivs_info, ivs_ioctl_param_t *ivs_param,
                 unsigned char **H, unsigned char **S, unsigned char **I)
{
	int ret;
	ret = ioctl(ivs_info.fd, IVS_HSI_CONVERSION, ivs_param);

	*H = ivs_info.mmap_addr + ivs_param->output_rlt_1.addr *
                              ivs_param->mem_info.memory_size;
	*S = ivs_info.mmap_addr + ivs_param->output_rlt_2.addr *
                              ivs_param->mem_info.memory_size;
	*I = ivs_info.mmap_addr + ivs_param->output_rlt_3.addr *
                              ivs_param->mem_info.memory_size;

    return ret ;
}

int operationRgb(ivs_info_t ivs_info, ivs_ioctl_param_t *ivs_param,
                 unsigned char **R, unsigned char **G, unsigned char **B)
{
	int ret;
	ret = ioctl(ivs_info.fd, IVS_RGB_CONVERSION, ivs_param);

	*R = ivs_info.mmap_addr + ivs_param->output_rlt_1.addr *
                              ivs_param->mem_info.memory_size;
	*G = ivs_info.mmap_addr + ivs_param->output_rlt_2.addr *
                              ivs_param->mem_info.memory_size;
	*B = ivs_info.mmap_addr + ivs_param->output_rlt_3.addr *
                              ivs_param->mem_info.memory_size;

    return ret ;
}

int operationIntegralImage(ivs_info_t ivs_info, ivs_ioctl_param_t *ivs_param,
	                       unsigned char **II)   
{
	int ret;
	ret = ioctl(ivs_info.fd, IVS_INTEGRAL_IMAGE, ivs_param);

	*II = ivs_info.mmap_addr + ivs_param->output_rlt_1.addr *
                               ivs_param->mem_info.memory_size;
    
	return ret;
}

int operationSquaredIntegralImage(ivs_info_t ivs_info, ivs_ioctl_param_t *ivs_param,
	                              unsigned char **BII)   
{
	int ret;
	ret = ioctl(ivs_info.fd, IVS_SQUARED_INTEGRAL_IMAGE, ivs_param);

	*BII = ivs_info.mmap_addr + ivs_param->output_rlt_1.addr *
                                ivs_param->mem_info.memory_size;
    
	return ret;
}

int operationDeInterleaving(ivs_info_t ivs_info, ivs_ioctl_param_t *ivs_param,
	                        unsigned char **Y, unsigned char **CBCR)
{
	int ret;
	ret = ioctl(ivs_info.fd, IVS_DE_INTERLEAVING, ivs_param);	

	*Y    = ivs_info.mmap_addr + ivs_param->output_rlt_1.addr *
                                 ivs_param->mem_info.memory_size;
	*CBCR = ivs_info.mmap_addr + ivs_param->output_rlt_2.addr *
                                 ivs_param->mem_info.memory_size;
		
	return ret ;
}

int operationHistogram(ivs_info_t ivs_info, ivs_ioctl_param_t *ivs_param,
	                   unsigned char **HI)   
{
	int ret;
	ret = ioctl(ivs_info.fd, IVS_HISTOGRAM, ivs_param);

	*HI = ivs_info.mmap_addr + ivs_param->output_rlt_1.addr *
                               ivs_param->mem_info.memory_size;
    
	return ret;
}

int operationConvolution(ivs_info_t ivs_info, ivs_ioctl_param_t *ivs_param,
                         unsigned char **H, unsigned char **S, unsigned char **I)
{
	int i, ret;

    for (i = 0; i < 25; i++)
        ivs_param->operation_info.template_element[i] = sobel[i];
    
	ret = ioctl(ivs_info.fd, IVS_CONVOLUTION, ivs_param);

	*H = ivs_info.mmap_addr + ivs_param->output_rlt_1.addr *
                              ivs_param->mem_info.memory_size;
	*S = ivs_info.mmap_addr + ivs_param->output_rlt_2.addr *
                              ivs_param->mem_info.memory_size;
	*I = ivs_info.mmap_addr + ivs_param->output_rlt_3.addr *
                              ivs_param->mem_info.memory_size;
    
	return ret ;
}

int operationMorphology(ivs_info_t ivs_info, ivs_ioctl_param_t *ivs_param,
                        unsigned char **H)
{
    int i, ret;

    for (i = 0; i < 25; i++)
        ivs_param->operation_info.template_element[i] = morph[i];

    ret = ioctl(ivs_info.fd, IVS_MORPHOLOGY, ivs_param);

	*H = ivs_info.mmap_addr + ivs_param->output_rlt_1.addr *
                              ivs_param->mem_info.memory_size;
    
	return ret ;
}

int operationSad(ivs_info_t ivs_info, ivs_ioctl_param_t *ivs_param,
	             unsigned char **SAD)
{
	int ret;
	ret = ioctl(ivs_info.fd, IVS_SAD, ivs_param);

	*SAD = ivs_info.mmap_addr + ivs_param->output_rlt_1.addr *
                                ivs_param->mem_info.memory_size;
    
	return ret;
}

int operationRaster(ivs_info_t ivs_info, ivs_ioctl_param_t *ivs_param,
	                unsigned char **ROS)
{
	int ret;
	ret = ioctl(ivs_info.fd, IVS_RASTER_OPERATION, ivs_param);

	*ROS = ivs_info.mmap_addr + ivs_param->output_rlt_1.addr *
                                ivs_param->mem_info.memory_size;
    
	return ret;
}

int operationCascaded(ivs_info_t ivs_info, ivs_ioctl_param_t *ivs_param,
	                  unsigned char **CC)
{
    int ret;
	ret = ioctl(ivs_info.fd, IVS_CASCADED_CLASSIFIER, ivs_param);
    
	*CC = ivs_info.mmap_addr + ivs_param->output_rlt_1.addr *
                               ivs_param->mem_info.memory_size;
    
	return ret;
}

int readImg(ivs_info_t ivs_info, ivs_ioctl_param_t *ivs_param, 
            char *input_fn, int file_idx)
{    
	int img_width   = ivs_param->img_info.img_width;
	int img_height  = ivs_param->img_info.img_height;
	int file_format = ivs_param->img_info.img_format;
    int offset      = 0; 
    int data_size   = 0;
    int fret        = 0;
    
    FILE *file         = NULL ;
	unsigned char *ICH = NULL ;

    if (file_idx == 1)
        offset = ivs_param->input_img_1.addr * 
                 ivs_param->mem_info.memory_size;
    else
        offset = ivs_param->input_img_2.addr * 
                 ivs_param->mem_info.memory_size;

    if (file_format == 0)
        data_size = img_width * img_height / 8 ;
    else
        data_size = img_width * img_height * file_format ;

    ICH = (unsigned char *)malloc(sizeof(unsigned char) * data_size) ;
	if (ICH == NULL) {
		printf("allocate memory error\n") ;
		return -1;
	}
                                                          
    if (input_fn != NULL) {
        file = fopen(input_fn, "rb");
        if (file == NULL) {
            printf("file is not exit \n");
            return -1;
        }

        fret = fread(ICH, 1, data_size, file);
        if (fret != data_size) {
		    printf("image file error, file read 0x%x\n", fret);
			return -1;
	    }

        fclose(file); 
    }

    if (file_idx == 1) {
        memcpy(ivs_info.mmap_addr + offset, ICH, data_size);
        ivs_param->input_img_1.size = data_size ;
    } else {
        memcpy(ivs_info.mmap_addr + offset, ICH, data_size);
        ivs_param->input_img_2.size = data_size ;
    }

    free(ICH);
          
	return 0;
}

int driverInit(ivs_info_t *ivs_info, ivs_ioctl_param_t *ivs_param)
{
	int ret=0;
	unsigned int API_version = IVS_VER;
	
	//	open node
	ivs_info->fd = open(IVS_DEV, O_RDWR);
	if (ivs_info->fd < 0) {
    	printf("Fail to open %s\n", IVS_DEV);
 		return -1;   	
    }
	
	//	mmap
	ivs_info->mmap_addr = (int)mmap(0, 
                           			IMEM_SIZE, 
                           			PROT_READ | PROT_WRITE, 
                           			MAP_SHARED, 
                           			ivs_info->fd, 
                           			0);
	if (ivs_info->mmap_addr == -1) {
		printf("Fail to use mmap, mmap_addr = %x!\n", ivs_info->mmap_addr);
		return -1;
	}
	
	//	check ap version
	ret = ioctl(ivs_info->fd, IVS_INIT, &API_version) ;	
	if(ret < 0)
		return -1;

    //	initial memory
	ret = ioctl(ivs_info->fd, IVS_INIT_MEM, ivs_param) ;	
	if(ret < 0)
		return -1;
	
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
 *            		-1 : unknown ioctl
 *                  -2 : copy from user error
 *            		-3 : copy to user error
 *            		-4 : ivs engine error
 *                  -5 : software time out 
 *  See also: 
 *                                                                        
 **************************************************************************/
int main(int argc, char **argv)
{
    int i = 0;
    int ret = 0;
    int offset = 0;

    //  ivs_driver parameter
    ivs_info_t ivs_info;
    ivs_ioctl_param_t ivs_param;

    //  output pointer
	unsigned char *result_1 = NULL, *result_2 = NULL, 
	                               	*result_3 = NULL;
#if (GOLDEN_COMPARE== 1)
    char FN[20];	
    FILE *FP = NULL;		
#endif

    //  receive command line parameters 
	char *input_fn_1            = NULL;
	char *input_fn_2            = NULL;
	int img_width               = 0;
	int img_height              = 0; 
	int img_format              = 0;
    int swap_y_cbcr             = 0;
    int swap_endian             = 0; 

    int operation               = 0;
    int kernel_template_index   = 0;
    int shifted_convolution_sum = 0;
    int convolution_tile        = 0;
    int morphology_operator     = 0;
    int select_threshold_output = 0; 
    int threshold               = 0;
    int block_size              = 0;
    int raster_operationo_code  = 0;
    int integral_cascade        = 0; 
       
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
     	else if( (strcmp(argv[i], "-if") == 0) && (++i < argc) )
			img_format = atoi(argv[i]);
     	else if( (strcmp(argv[i], "-op") == 0) && (++i < argc) )
			operation = atoi(argv[i]);
     	else if( (strcmp(argv[i], "-kt") == 0) && (++i < argc) )
			kernel_template_index = atoi(argv[i]);
     	else if( (strcmp(argv[i], "-sh") == 0) && (++i < argc) )
			shifted_convolution_sum = atoi(argv[i]);
     	else if( (strcmp(argv[i], "-ti") == 0) && (++i < argc) )
			convolution_tile = atoi(argv[i]);
      	else if( (strcmp(argv[i], "-mp") == 0) && (++i < argc) )
			morphology_operator = atoi(argv[i]);
     	else if( (strcmp(argv[i], "-st") == 0) && (++i < argc) )
			select_threshold_output = atoi(argv[i]);
      	else if( (strcmp(argv[i], "-th") == 0) && (++i < argc) )
			threshold = atoi(argv[i]);
		else if( (strcmp(argv[i], "-bl") == 0) && (++i < argc) )
			block_size = atoi(argv[i]);
		else if( (strcmp(argv[i], "-ro") == 0) && (++i < argc) )
			raster_operationo_code = atoi(argv[i]);
		else if( (strcmp(argv[i], "-ic") == 0) && (++i < argc) )
			integral_cascade = atoi(argv[i]);        
		else if( (strcmp(argv[i], "-sw") == 0) && (++i < argc) )
			swap_y_cbcr = atoi(argv[i]);
		else if( (strcmp(argv[i], "-en") == 0) && (++i < argc) )
			swap_endian = atoi(argv[i]);
		else {
		  	printf ("unknow parameter %s\n", argv[i]);
			usage();
			return -1;
		}
		i++ ;
	}

    memset(&ivs_param, 0, sizeof(struct ivs_param_t));

	ivs_param.img_info.img_width   = img_width;
	ivs_param.img_info.img_height  = img_height;
	ivs_param.img_info.img_format  = img_format;
	ivs_param.img_info.swap_y_cbcr = swap_y_cbcr;
	ivs_param.img_info.swap_endian = swap_endian;
    
  	ivs_param.operation_info.operation               = operation;
	ivs_param.operation_info.kernel_template_index   = kernel_template_index;
	ivs_param.operation_info.shifted_convolution_sum = shifted_convolution_sum;
	ivs_param.operation_info.morphology_operator     = morphology_operator;
	ivs_param.operation_info.select_threshold_output = select_threshold_output;
	ivs_param.operation_info.threshold               = threshold;
	ivs_param.operation_info.block_size              = block_size;
	ivs_param.operation_info.raster_operationo_code  = raster_operationo_code;
	ivs_param.operation_info.integral_cascade	     = integral_cascade;
    			
    //  check parameter
    ret = check_parameter(ivs_param,input_fn_1,input_fn_2);
    if (ret < 0)
    	return ret;

	//  initial_driver
	ivs_param.img_info.img_width  = max_width;
    ivs_param.img_info.img_height = max_height;
	ret = driverInit(&ivs_info,&ivs_param);
	if (ret < 0)
	    return ret;
    ivs_param.img_info.img_width  = img_width;
    ivs_param.img_info.img_height = img_height;

    //  open & read input image
    ivs_param.input_img_1.addr = 0;
	ret = readImg(ivs_info, &ivs_param, input_fn_1, 1);
	if (ret < 0)
		goto error;

	ivs_param.input_img_2.addr = 1;
	ret = readImg(ivs_info, &ivs_param, input_fn_2, 2);
	if (ret < 0)
		goto error;
    
    ivs_param.input_img_1.offset = offset;
    ivs_param.input_img_2.offset = offset;
    ivs_param.output_rlt_1.offset = offset;
    ivs_param.output_rlt_2.offset = offset;
    ivs_param.output_rlt_3.offset = offset;

	ivs_param.output_rlt_1.addr = 2;
	ivs_param.output_rlt_2.addr = 3;
	ivs_param.output_rlt_3.addr = 4;	
    switch (operation) {
        case 0:
            ret = operationHsi(ivs_info, &ivs_param,
			                   &result_1, &result_2, &result_3);
		    if (ret == -2) {
		        printf("hsi conversion copy from user error %d %d\n",
		               img_height,img_width);
                goto error;
            } else if (ret == -3) {
		        printf("hsi conversion copy to user error %d %d\n",
		               img_height,img_width);
                goto error;
            } else if (ret == -4) {
	 		    printf("hsi conversion software time out %d %d\n",
                       img_height,img_width);
                goto error;
            } else {
                printf("hsi conversion success %d %d\n",
                       img_height,img_width);
#if (GOLDEN_COMPARE== 1)
                sprintf(FN,"t_h_%d_%d",img_height,img_width);
                FP = fopen(FN,"wb");
                fwrite(result_1,1,ivs_param.output_rlt_1.size,FP);
                fclose(FP);

                sprintf(FN,"t_s_%d_%d",img_height,img_width);
                FP = fopen(FN,"wb");
			    fwrite(result_2,1,ivs_param.output_rlt_2.size,FP);
			    fclose(FP);

                sprintf(FN,"t_i_%d_%d",img_height,img_width);
                FP = fopen(FN,"wb");
                fwrite(result_3,1,ivs_param.output_rlt_3.size,FP);
			    fclose(FP);
#endif
            }
        break;

        case 1:
            ret = operationRgb(ivs_info, &ivs_param,
			                   &result_1, &result_2, &result_3);
		    if (ret == -2) {
			    printf("rgb conversion copy from user error %d %d\n",
			           img_height,img_width);
                goto error;
            } else if (ret == -3) {
			    printf("rgb conversion copy to user error %d %d\n",
			           img_height,img_width);
                goto error;
            } else if (ret == -4) {
	 		    printf("rgb conversion software time out %d %d\n",
                       img_height,img_width);
                goto error;
            } else {
                printf("rgb conversion success %d %d\n",
                       img_height,img_width) ;
#if (GOLDEN_COMPARE== 1)
                sprintf(FN,"t_r_%d_%d",img_height,img_width);
                FP = fopen(FN,"wb");
                fwrite(result_1,1,ivs_param.output_rlt_1.size,FP);
                fclose(FP);

                sprintf(FN,"t_g_%d_%d",img_height,img_width);
                FP = fopen(FN,"wb");
                fwrite(result_2,1,ivs_param.output_rlt_2.size,FP) ;
                fclose(FP) ;

                sprintf(FN,"t_b_%d_%d",img_height,img_width);
                FP = fopen(FN,"wb") ;
                fwrite(result_3,1,ivs_param.output_rlt_3.size,FP) ;
                fclose(FP) ;
#endif
            }
        break;

        case 2:
            ret = operationIntegralImage(ivs_info, &ivs_param,
			                             &result_1);		
		    if (ret == -2) {
			    printf("integral image copy from user error %d %d\n",
                       img_height,img_width);
                goto error;
            } else if (ret == -3) {
			    printf("integral image copy to user error %d %d\n",
                       img_height,img_width);
                goto error;
            } else if (ret == -4) {
	 		    printf("integral image software time out %d %d\n",
                       img_height,img_width);
                goto error;
            } else {
                printf("integral image success %d %d\n",
                       img_height,img_width);
#if (GOLDEN_COMPARE== 1)
                sprintf(FN,"t_ii_%d_%d",img_height,img_width);                            
			    FP = fopen(FN,"wb");			
			    fwrite(result_1,1,ivs_param.output_rlt_1.size,FP) ;
			    fclose(FP);	
#endif
            }
        break;

        case 3:
            ret = operationSquaredIntegralImage(ivs_info, &ivs_param,
			                                    &result_1);
		    if (ret == -2) {
			    printf("squared integral image copy from user error %d %d\n",
                       img_height,img_width);
                goto error;
            } else if (ret == -3) {
			    printf("squared integral image copy to user error %d %d\n",
                       img_height,img_width);
                goto error;
            } else if (ret == -4) {
	 		    printf("squared integral image software time out %d %d\n",
                       img_height,img_width);
                goto error;
            } else {
                printf("squared integral image success %d %d\n",
                       img_height,img_width);
#if (GOLDEN_COMPARE== 1)
                sprintf(FN,"t_sii_%d_%d",img_height,img_width);
			    FP = fopen(FN,"wb") ;
    			fwrite(result_1,1,ivs_param.output_rlt_1.size,FP) ;
			    fclose(FP) ;	
#endif
	        }
        break;

        case 4:
            ret = operationDeInterleaving(ivs_info, &ivs_param,
			                              &result_1, &result_2);
		    if (ret == -2) {
			    printf("de interleaving copy from user error %d %d\n",
			           img_height,img_width);
                goto error;
            } else if (ret == -3) {
			    printf("de interleaving copy to user error %d %d\n",
			           img_height,img_width);
                goto error;
            } else if (ret == -4) {
	 		    printf("de interleaving software time out %d %d\n",
                       img_height,img_width);
                goto error;
            } else {
                printf("de interleaving success %d %d\n",
                       img_height,img_width);
#if (GOLDEN_COMPARE== 1)
                sprintf(FN,"t_y_%d_%d",img_height,img_width);
                FP = fopen(FN,"wb");
                fwrite(result_1,1,ivs_param.output_rlt_1.size,FP);
                fclose(FP);

                sprintf(FN,"t_cbcr_%d_%d",img_height,img_width);
                FP = fopen(FN,"wb");
                fwrite(result_2,1,ivs_param.output_rlt_2.size,FP);
                fclose(FP);	
#endif
            }		
        break;

        case 5:
            ret = operationHistogram(ivs_info, &ivs_param,
			                         &result_1);
		    if (ret == -2) {
			    printf("histogram copy from user error %d %d\n",
                       img_height,img_width);
                goto error;
            } else if (ret == -3) {
			    printf("histogram copy to user error %d %d\n",
                       img_height,img_width);
                goto error;
            } else if (ret == -4) {
	 		    printf("histogram software time out    %d %d\n",
                       img_height,img_width);
                goto error;
            } else {
                printf("histogram software success    %d %d\n",
                       img_height,img_width);
#if (GOLDEN_COMPARE== 1)
                sprintf(FN,"t_hi_%d_%d",img_height,img_width);
                FP = fopen(FN,"wb"); 
                fwrite(result_1,1,ivs_param.output_rlt_1.size,FP);
                fclose(FP);
#endif
            }
        break;

        case 6:
            ret = operationConvolution(ivs_info, &ivs_param,
			                           &result_1, &result_2, &result_3);
		    if (ret == -2) {
			    printf("convolution copy from user error %d %d\n",
                       img_height,img_width);
                goto error;
            } else if (ret == -3) {
			    printf("convolution copy to user error %d %d\n",
                       img_height,img_width);
                goto error;
            } else if (ret == -4) {
	 		    printf("convolution software time out    %d %d\n",
	 		           img_height,img_width);
                goto error;
            } else {
                printf("convolution software success    %d %d\n",
	 		           img_height,img_width);
#if (GOLDEN_COMPARE== 1)
                sprintf(FN,"t_h_%d_%d",img_height,img_width);
                FP = fopen(FN,"wb");
                fwrite(result_1,1,ivs_param.output_rlt_1.size,FP);
                fclose(FP);

                sprintf(FN,"t_s_%d_%d",img_height,img_width);
                FP = fopen(FN,"wb"); 
		        fwrite(result_2,1,ivs_param.output_rlt_2.size,FP);    
			    fclose(FP);

                sprintf(FN,"t_i_%d_%d",img_height,img_width);
                FP = fopen(FN,"wb"); 
                fwrite(result_3,1,ivs_param.output_rlt_3.size,FP);
                fclose(FP);
#endif
            }
        break;

        case 7:
            ret = operationMorphology(ivs_info, &ivs_param,
			                          &result_1);
		    if (ret == -2) {
			    printf("morphology copy from user error %d %d\n",
                       img_height,img_width);
                goto error;
            } if (ret == -3) {
			    printf("morphology copy to user error %d %d\n",
                       img_height,img_width);
                goto error;
            } else if (ret == -4) {
	 		    printf("morphology software time out    %d %d\n",
                       img_height,img_width);
                goto error;
            } else {
                printf("morphology software success    %d %d\n",
                       img_height,img_width);
#if (GOLDEN_COMPARE== 1)
                sprintf(FN,"t_mor_%d_%d_%d",img_height,img_width,
                                            morphology_operator);
                FP = fopen(FN,"wb"); 
                fwrite(result_1,1,ivs_param.output_rlt_1.size,FP);
                fclose(FP);
#endif
            }
        break;

        case 8:
            ret = operationSad(ivs_info, &ivs_param,
			                   &result_1);			
		    if (ret == -2) {
			    printf("sad copy from user error %d %d\n",
			           img_height,img_width);
                goto error;
            } else if (ret == -3) {
			    printf("sad copy to user error %d %d\n",
			           img_height,img_width);
                goto error;
            } else if (ret == -4) {
	 		    printf("sad software time out %d %d\n",
                       img_height,img_width);
                goto error;
            } else {
                printf("sad software success %d %d\n",
                       img_height,img_width);
#if (GOLDEN_COMPARE== 1)
                sprintf(FN,"t_sad_%d_%d_%d_%d",img_height,img_width,
                                               block_size,threshold);                
			    FP = fopen(FN,"wb");
                fwrite(result_1,1,ivs_param.output_rlt_1.size,FP);
                fclose(FP);
#endif
            }
        break;

        case 9:
            ret = operationRaster(ivs_info, &ivs_param,
			                      &result_1);			
		    if (ret == -2) {
			    printf("roster copy from user error %d %d\n",
                       img_height,img_width);
                goto error;
            } else if (ret == -3) {
			    printf("roster copy to user error %d %d\n",
                       img_height,img_width);
                goto error;
            } else if (ret == -4) {
	 		    printf("roster software time out %d %d\n",
                       img_height,img_width);
                goto error;
            } else {
                printf("roster software success %d %d\n",
                       img_height,img_width);
#if (GOLDEN_COMPARE== 1)
                sprintf(FN,"t_rast_%d_%d_%d",img_height,img_width,
                                             raster_operationo_code);  	
			    FP = fopen(FN,"wb");
                fwrite(result_1,1,ivs_param.output_rlt_1.size,FP);
                fclose(FP);
#endif
	        }
        break;

        case 10:
            ret = operationCascaded(ivs_info, &ivs_param,
			                        &result_1);
		    if (ret == -2) {
			    printf("cascade copy from user error %d %d\n", 
                       img_height,img_width);
                goto error;
            } else if (ret == -3) {
			    printf("cascade copy to user error %d %d\n", 
                       img_height,img_width);
                goto error;
            } else if (ret == -4) {
	 		    printf("cascade software time out %d %d\n",
                       img_height,img_width);
                goto error;
            } else {
                printf("cascade software success %d %d\n",
                       img_height,img_width);
#if (GOLDEN_COMPARE== 1)
                sprintf(FN,"t_rast_%d_%d",img_height,img_width);                  
			    FP = fopen(FN,"wb") ;
                fwrite(result_1,1,ivs_param.output_rlt_1.size,FP);          
			    fclose(FP);
#endif
	        }
        break;

        default:
            printf("undefined operation %d\n", operation);
        break;
    }
	    
error:
	driverClose(ivs_info);
    
	return ret;
}

