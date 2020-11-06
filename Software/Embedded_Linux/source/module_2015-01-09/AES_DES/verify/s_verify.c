/**
 * @file s_test.c
 * AES_DES Sample Verify Application
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "security.h"

//#define SECURITY_RANDOM_KEY_AND_IV

#define DMA_BUFFER_SIZE     (4<<20)     ///< 4MByte
#define DATA_DUMMY_PAD_LEN  16          ///< 16Byte

#define DES_BLOCK_SIZE      8           ///< 8Byte
#define AES_BLOCK_SIZE      16          ///< 16Byte

typedef enum {
    SECURITY_ALG_DES = 0,
    SECURITY_ALG_TRIPLE_DES,
    SECURITY_ALG_AES_128,
    SECURITY_ALG_AES_192,
    SECURITY_ALG_AES_256,
    SECURITY_ALG_MAX
} SECURITY_ALG_T;

typedef enum {
    SECURITY_MODE_ECB = 0,
    SECURITY_MODE_CBC,
    SECURITY_MODE_CTR,
    SECURITY_MODE_CFB,
    SECURITY_MODE_OFB,
   SECURITY_MODE_MAX
} SECURITY_MODE_T;

typedef enum {
    SECURITY_KEY_FIX = 0,
    SECURITY_KEY_RANDOM,
    SECURITY_KEY_MAX
} SECURITY_KEY_T;

static unsigned int g_des_64_key[] = { ///< 64bits
    0x01010101,
    0x01010101
};

static unsigned int g_aes_128_key[] = { ///< 128Bits
    0x2b7e1516,
    0x28aed2a6,
    0xabf71588,
    0x09cf4f3c
};

static unsigned int g_aes_192_key[] = { ///< 192Bits
    0x8e73b0f7,
    0xda0e6452,
    0xc810f32b,
    0x809079e5,
    0x62f8ead2,
    0x522c6b7b
};

static unsigned int g_aes_256_key[] = { ///< 256Bits
    0x603deb10,
    0x15ca71be,
    0x2b73aef0,
    0x857d7781,
    0x1f352c07,
    0x3b6108d7,
    0x2d9810a3,
    0x0914dff4
};

static unsigned int g_initial_vector[] = {
    0x00010203,
    0x04050607,
    0x08090a0b,
    0x0c0d0e0f
};

static unsigned int g_initial_vector_ctr[] = {
    0xf0f1f2f3,
    0xf4f5f6f7,
    0xf8f9fafb,
    0xfcfdfeff
};

static void print_help_msg(void)
{
    fprintf(stdout, "\nUsage: s_verify <alg> <mode> <key_type> <dbg_out> <test_file>"
                    "\n                <alg>      0:DES 1:Triple_DES 2:AES_128 3:AES_192 4:AES_256"
                    "\n                <mode>     0:ECB 1:CBC 2:CTR 3:CFB 4:OFB"
                    "\n                <dbg_out>  0:Disable 1:Enable"
                    "\n                <key_type> 0:FIX 1:Random"
                    "\n");
}

int get_key_and_iv(esreq *wrq)
{
    int i;

    if(!wrq)
        return -1;

    /* clear */
    memset(wrq->key_addr, 0, sizeof(unsigned int)*8);
    memset(wrq->IV_addr,  0, sizeof(unsigned int)*4);

    /* Key */
    switch(wrq->algorithm) {
        case Algorithm_DES:
        case Algorithm_Triple_DES:
            for(i=0; i<(sizeof(g_des_64_key)/sizeof(g_des_64_key[0])); i++) {
                 if(i >= 8)
                    break;
                 wrq->key_addr[i] = g_des_64_key[i];
                 wrq->key_length+=4;
            }
            break;
        case Algorithm_AES_128:
            for(i=0; i<(sizeof(g_aes_128_key)/sizeof(g_aes_128_key[0])); i++) {
                 if(i >= 8)
                    break;
                 wrq->key_addr[i] = g_aes_128_key[i];
                 wrq->key_length+=4;
            }
            break;
        case Algorithm_AES_192:
            for(i=0; i<(sizeof(g_aes_192_key)/sizeof(g_aes_192_key[0])); i++) {
                 if(i >= 8)
                    break;
                 wrq->key_addr[i] = g_aes_192_key[i];
                 wrq->key_length+=4;
            }
            break;
        case Algorithm_AES_256:
            for(i=0; i<(sizeof(g_aes_256_key)/sizeof(g_aes_256_key[0])); i++) {
                 if(i >= 8)
                    break;
                 wrq->key_addr[i] = g_aes_256_key[i];
                 wrq->key_length+=4;
            }
            break;
        default:
            return -1;
    }

    /* Initial Vector */
    if(wrq->mode == CTR_mode) {
        for(i=0; i<(sizeof(g_initial_vector_ctr)/sizeof(g_initial_vector_ctr[0])); i++) {
             if(i >= 4)
                break;
             wrq->IV_addr[i] = g_initial_vector_ctr[i];
             wrq->IV_length+=4;
        }
    }
    else {
        for(i=0; i<(sizeof(g_initial_vector)/sizeof(g_initial_vector[0])); i++) {
             if(i >= 4)
                break;
             wrq->IV_addr[i] = g_initial_vector[i];
             wrq->IV_length+=4;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    int alg, mode, key_type, dbg_out, ret = 0;
    esreq wrq;
    unsigned int dma_size;
    struct stat fileStat;
    char        *file_path = NULL;
    int          file_fd   = 0;
    int          dev_fd    = 0;
    unsigned int file_va   = 0;
    unsigned int dma_va    = 0;

    /* check input arguments */
    if(argc < 6) {
        print_help_msg();
        return -EPERM;
    }

    /* get input arguments */
    alg = atoi(argv[1]);
    if(alg < 0 || alg >= SECURITY_ALG_MAX) {
        print_help_msg();
        return -EPERM;
    }

    mode = atoi(argv[2]);
    if(mode < 0 || mode >= SECURITY_MODE_MAX) {
        print_help_msg();
        return -EPERM;
    }

    key_type = atoi(argv[3]);
    if(key_type < 0 || key_type >= SECURITY_KEY_MAX) {
        print_help_msg();
        return -EPERM;
    }

#ifndef SECURITY_RANDOM_KEY_AND_IV
    key_type = SECURITY_KEY_FIX;    ///< test and compare need to use fix key and IV
#endif

    dbg_out = atoi(argv[4]);
    if(dbg_out < 0 || dbg_out > 1) {
        print_help_msg();
        return -EPERM;
    }

    file_path = argv[5];

    /* check algorithm and mode */
    if((alg == SECURITY_ALG_DES || alg == SECURITY_ALG_TRIPLE_DES) && (mode == SECURITY_MODE_CTR)) { ///< DES not support CTR mode
        printf("DES not support CTR mode\n");
        return -1;
    }

    /* open security device */
    if((dev_fd = open("/dev/security", O_RDWR)) < 0) {
        printf("open /dev/security fail\n");
        return -1;
    }

    /* open file */
    if((file_fd = open(file_path, O_RDONLY)) < 0) {
        printf("error to open %s !!\n", file_path);
        ret = -1;
        goto exit;
    }

    /* get file stat */
    if(fstat(file_fd, &fileStat) < 0) {
        printf("error to stat %s\n", file_path);
        ret = -1;
        goto err;
    }

    /* check file size */
    if(alg == SECURITY_ALG_DES || alg == SECURITY_ALG_TRIPLE_DES) {  ///< DES or Triple_DES
        if(fileStat.st_size%DES_BLOCK_SIZE != 0) {
            printf("error! file size(%u) must mutiple of %d\n", (unsigned int)fileStat.st_size, DES_BLOCK_SIZE);
            ret = -1;
            goto err;
        }
    }
    else {
        if(fileStat.st_size%AES_BLOCK_SIZE != 0) {
            printf("error! file size(%u) must mutiple of %d\n", (unsigned int)fileStat.st_size, AES_BLOCK_SIZE);
            ret = -1;
            goto err;
        }
    }

    /* memory map to file */
    file_va = (unsigned int)mmap(NULL, fileStat.st_size, PROT_READ, MAP_SHARED, file_fd, 0);
    if((int)file_va == -1) {
        printf("mmap failed!!\n");
        ret = -1;
        goto err;
    }

    dma_size = (fileStat.st_size+DATA_DUMMY_PAD_LEN)*2;
    dma_va   = (unsigned int)mmap(NULL, dma_size, PROT_READ|PROT_WRITE, MAP_SHARED, dev_fd, 0);
	if((int)dma_va == -1) {
	    printf("mmap failed!!\n");
        ret = -1;
	    goto err;
	}

	memset(&wrq, 0, sizeof(esreq));

	switch(alg) {
	    case SECURITY_ALG_DES:
	        wrq.algorithm = Algorithm_DES;
	        break;
	    case SECURITY_ALG_TRIPLE_DES:
	        wrq.algorithm = Algorithm_Triple_DES;
	        break;
	    case SECURITY_ALG_AES_128:
	        wrq.algorithm = Algorithm_AES_128;
	        break;
	    case SECURITY_ALG_AES_192:
	        wrq.algorithm = Algorithm_AES_192;
	        break;
	    case SECURITY_ALG_AES_256:
	        wrq.algorithm = Algorithm_AES_256;
	        break;
	    default:
	        ret = -1;
	        goto err;
	}

	switch(mode) {
	    case SECURITY_MODE_ECB:
	        wrq.mode = ECB_mode;
	        break;
	    case SECURITY_MODE_CBC:
	        wrq.mode = CBC_mode;
	        break;
	    case SECURITY_MODE_CTR:
	        wrq.mode = CTR_mode;
	        break;
	    case SECURITY_MODE_CFB:
	        wrq.mode = CFB_mode;
	        break;
	    case SECURITY_MODE_OFB:
	        wrq.mode = OFB_mode;
	        break;
	    default:
	        ret = -1;
	        goto err;
	}

	if(key_type == SECURITY_KEY_FIX) {
	    ret = get_key_and_iv(&wrq);
        if(ret < 0) {
            printf("fail to get key and initial vector!!\n");
            goto err;
        }
    }

	wrq.DataIn_addr  = dma_va;
    wrq.DataOut_addr = wrq.DataIn_addr + (fileStat.st_size + DATA_DUMMY_PAD_LEN);

    /* start encrypt */
    memcpy((void *)wrq.DataIn_addr, (void *)file_va, fileStat.st_size);
    wrq.data_length = fileStat.st_size + wrq.IV_length;
    if(ioctl(dev_fd, ((key_type == SECURITY_KEY_FIX) ? ES_ENCRYPT : ES_AUTO_ENCRYPT), &wrq) < 0) {
        printf("%s IOCTL fail!!\n", ((key_type == SECURITY_KEY_FIX) ? "ES_ENCRYPT" : "ES_AUTO_ENCRYPT"));
        ret = -1;
        goto err;
    }

    /* clear data_in buffer */
    memset((void *)wrq.DataIn_addr, 0, (fileStat.st_size + DATA_DUMMY_PAD_LEN));

    /* start dencrypt */
    wrq.DataIn_addr  = dma_va + (fileStat.st_size + DATA_DUMMY_PAD_LEN);
    wrq.DataOut_addr = dma_va;
    wrq.data_length  = fileStat.st_size + wrq.IV_length;
    if(ioctl(dev_fd, ((key_type == SECURITY_KEY_FIX) ? ES_DECRYPT : ES_AUTO_DECRYPT), &wrq) < 0) {
        printf("%s IOCTL fail!!\n", ((key_type == SECURITY_KEY_FIX) ? "ES_DECRYPT" : "ES_AUTO_DECRYPT"));
        ret = -1;
        goto err;
    }

    if(dbg_out) {
        int i;

        for(i=0; i<(fileStat.st_size+DATA_DUMMY_PAD_LEN)/4; i++) {
            if(i >= (fileStat.st_size/4))
                printf("\norg:%08x enc:%08x dec:%08x", 0, *((unsigned int *)(wrq.DataIn_addr+(i*4))), *((unsigned int *)(wrq.DataOut_addr+(i*4))));
            else
                printf("\norg:%08x enc:%08x dec:%08x", *((unsigned int *)(file_va+(i*4))), *((unsigned int *)(wrq.DataIn_addr+(i*4))), *((unsigned int *)(wrq.DataOut_addr+(i*4))));
        }
        printf("\n");
    }

    /* start data comapre */
    if(memcmp((void *)wrq.DataOut_addr, (void *)file_va, fileStat.st_size) != 0)
        printf("encrypt and dencrypt data compare(data_len:%lu)...fail!!\n", fileStat.st_size);
    else
        printf("encrypt and dencrypt data compare(data_len:%lu)...OK!!\n",   fileStat.st_size);

err:
    if(dma_va)
        munmap((void *)dma_va, dma_size);

    if(file_va)
        munmap((void *)file_va, fileStat.st_size);

    if(file_fd)
        close(file_fd);

exit:
    if(dev_fd)
        close(dev_fd);

    return ret;
}
