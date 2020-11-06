/**
 * @file security_test.c
 * AES_DES Sample Test Application
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

#define SECURITY_ALG        Algorithm_AES_256
#define SECURITY_MODE       CBC_mode

#define SIZE_1K             (1  << 10)
#define SIZE_64K            (64 << 10)
#define SIZE_1M             (1  << 20)
#define SIZE_2M             (2  << 20)
#define SIZE_4M             (4  << 20)

#define DMA_BUFFER_SIZE     SIZE_64K
//#define TEST_MULTI_PROCESS

#ifdef  TEST_MULTI_PROCESS
#define PROCESS_CNT 2
#else
#define PROCESS_CNT 1
#endif

extern int errno;

static unsigned int g_aes_128_key[] = {
    0x2b7e1516,
    0x28aed2a6,
    0xabf71588,
    0x09cf4f3c
};

static unsigned int g_aes_192_key[] = {
    0x8e73b0f7,
    0xda0e6452,
    0xc810f32b,
    0x809079e5,
    0x62f8ead2,
    0x522c6b7b
};

static unsigned int g_aes_256_key[] = {
    0x603deb10,
    0x15ca71be,
    0x2b73aef0,
    0x857d7781,
    0x1f352c07,
    0x3b6108d7,
    0x2d9810a3,
    0x0914dff4
};

static unsigned int g_des_key[] = {
    0x01010101,
    0x01010101
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
    printf("Usage:\n");
    printf("security_test -h\n");
    printf("security_test -k\n");
    printf("security_test -e\n");
    printf("security_test -d\n");
    printf("security_test -E\n");
    printf("security_test -D\n");
    printf("\n");
    printf("-k	get random key and initial vector\n");
    printf("-e	set algorithm,mode,argument to run encrypt function\n");
    printf("-d	set algorithm,mode,argument to run decrypt function\n");
    printf("-E	set algorithm,mode,argument, generate random key and IV to encrypt data\n");
    printf("-D	set algorithm,mode,argument, generate random key and IV to decrypt data\n");
    printf("\n");
}

/******************************* MAIN ********************************/
unsigned int get_ioctl_cmd(char* cmd)
{
    unsigned int ioctl_cmd = 0;

    if (strcmp(cmd, "-k") == 0)
    {
        ioctl_cmd = ES_GETKEY;
    }
    else if (strcmp(cmd, "-e") == 0)
    {
        ioctl_cmd = ES_ENCRYPT;
    }
    else if (strcmp(cmd, "-E") == 0)
    {
        ioctl_cmd = ES_AUTO_ENCRYPT;
    }
    else if (strcmp(cmd, "-d") == 0)
    {
        ioctl_cmd = ES_DECRYPT;
    }
    else if (strcmp(cmd, "-D") == 0)
    {
        ioctl_cmd = ES_AUTO_DECRYPT;
    }

    return ioctl_cmd;
}

int get_enc_input_data(esreq* wrq)
{
    if(!wrq || !wrq->DataIn_addr)
        return 0;

    *(unsigned int *)(wrq->DataIn_addr)      = 0xe2bec16b;
    *(unsigned int *)(wrq->DataIn_addr + 4)  = 0x969f402e;
    *(unsigned int *)(wrq->DataIn_addr + 8)  = 0x117e3de9;
    *(unsigned int *)(wrq->DataIn_addr + 12) = 0x2a179373;
    *(unsigned int *)(wrq->DataIn_addr + 16) = 0x578a2dae;
    *(unsigned int *)(wrq->DataIn_addr + 20) = 0x9cac031e;
    *(unsigned int *)(wrq->DataIn_addr + 24) = 0xac6fb79e;
    *(unsigned int *)(wrq->DataIn_addr + 28) = 0x518eaf45;
    *(unsigned int *)(wrq->DataIn_addr + 32) = 0x461cc830;
    *(unsigned int *)(wrq->DataIn_addr + 36) = 0x11e45ca3;
    *(unsigned int *)(wrq->DataIn_addr + 40) = 0x19c1fbe5;
    *(unsigned int *)(wrq->DataIn_addr + 44) = 0xef520a1a;
    *(unsigned int *)(wrq->DataIn_addr + 48) = 0x45249ff6;
    *(unsigned int *)(wrq->DataIn_addr + 52) = 0x179b4fdf;
    *(unsigned int *)(wrq->DataIn_addr + 56) = 0x7b412bad;
    *(unsigned int *)(wrq->DataIn_addr + 60) = 0x10376ce6;

    return 64;
}

int get_dec_input_data(esreq *wrq)
{
    if(!wrq || !wrq->DataIn_addr)
        return 0;

    *(unsigned int *)(wrq->DataIn_addr)      = 0x044c8cf5;
    *(unsigned int *)(wrq->DataIn_addr + 4)  = 0xbaf1e5d6;
    *(unsigned int *)(wrq->DataIn_addr + 8)  = 0xfbab9e77;
    *(unsigned int *)(wrq->DataIn_addr + 12) = 0xd6fb7b5f;
    *(unsigned int *)(wrq->DataIn_addr + 16) = 0x964efc9c;
    *(unsigned int *)(wrq->DataIn_addr + 20) = 0x8d80db7e;
    *(unsigned int *)(wrq->DataIn_addr + 24) = 0x7b779f67;
    *(unsigned int *)(wrq->DataIn_addr + 28) = 0x7d2c70c6;
    *(unsigned int *)(wrq->DataIn_addr + 32) = 0x6933f239;
    *(unsigned int *)(wrq->DataIn_addr + 36) = 0xcfbad9a9;
    *(unsigned int *)(wrq->DataIn_addr + 40) = 0x63e230a5;
    *(unsigned int *)(wrq->DataIn_addr + 44) = 0x61142304;
    *(unsigned int *)(wrq->DataIn_addr + 48) = 0xe205ebb2;
    *(unsigned int *)(wrq->DataIn_addr + 52) = 0xfce99bc3;
    *(unsigned int *)(wrq->DataIn_addr + 56) = 0x07196cda;
    *(unsigned int *)(wrq->DataIn_addr + 60) = 0x1b9d6a8c;

    return 64;
}

int get_key_and_iv(esreq *wrq)
{
    int i;

    if(!wrq)
        return -1;

    /* clear */
    memset(wrq->key_addr, 0, sizeof(unsigned int)*8);
    memset(wrq->IV_addr, 0, sizeof(unsigned int)*4);

    /* Key */
    switch(wrq->algorithm) {
        case Algorithm_DES:
        case Algorithm_Triple_DES:
            for(i=0; i<(sizeof(g_des_key)/sizeof(g_des_key[0])); i++) {
                 if(i >= 8)
                    break;
                 wrq->key_addr[i] = g_des_key[i];
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
    int i, j, k, ret = 0;
    unsigned int ioctl_cmd = 0;
    int          descript[PROCESS_CNT];
    unsigned int dma_size[PROCESS_CNT];
    esreq        wrq[PROCESS_CNT];
    int          test_length;

    if(argc < 2) {
        print_help_msg();
        return -1;
    }

    /* Special case for help... */
    if((!strncmp(argv[1], "-h", 9)) || (!strcmp(argv[1], "--help"))) {
        print_help_msg();
        return -1;
    }

    /* get ioctl cmd */
    ioctl_cmd = get_ioctl_cmd(argv[1]);
    if(ioctl_cmd == 0) {
        printf("invalid argv[1] %s\n", argv[1]);
        return -1;
    }

    /* open device */
    descript[0] = open("/dev/security", O_RDWR);
    if(descript[0] == 0) {
        printf("%s: open security device fail\n", argv[0]);
        return -1;
    }

    /* clear buffer */
    memset(wrq, 0, sizeof(esreq)*PROCESS_CNT);

    dma_size[0]         = DMA_BUFFER_SIZE*2;
	wrq[0].DataIn_addr  = (unsigned int) mmap(NULL, dma_size[0], PROT_READ|PROT_WRITE, MAP_SHARED, descript[0], 0);
    wrq[0].DataOut_addr = wrq[0].DataIn_addr + DMA_BUFFER_SIZE;
	if((int)wrq[0].DataIn_addr == -1) {
	    printf("mmap failed");
        ret = -1;
	    goto exit;
	}

    if(PROCESS_CNT == 2) {
        descript[1] = open("/dev/security", O_RDWR);
        if(descript[1] == 0) {
            printf("%s: open device fail\n", argv[0]);
            ret = -1;
            goto exit;
        }

        dma_size[1]         = DMA_BUFFER_SIZE;
	    wrq[1].DataIn_addr  = (unsigned int) mmap(NULL, dma_size[1], PROT_READ|PROT_WRITE, MAP_SHARED, descript[1], 0);
	    wrq[1].DataOut_addr = wrq[1].DataIn_addr;
	    if((int)wrq[1].DataIn_addr == -1) {
	        printf("mmap failed");
            ret = -1;
	        goto exit;
	    }
    }

    for(k=0; k<PROCESS_CNT; k++) {
        wrq[k].algorithm = SECURITY_ALG;
        wrq[k].mode      = SECURITY_MODE;

        if (ioctl_cmd != ES_GETKEY) {
            /* ========================== test pattern ===========================================*/
            ret = get_key_and_iv(&wrq[k]);
            if(ret < 0) {
                printf("wrq %d fail to get key and initial vector\n", k);
                goto exit;
            }

            test_length = 0;
            if ((ioctl_cmd == ES_ENCRYPT) || (ioctl_cmd == ES_AUTO_ENCRYPT))
                test_length = get_enc_input_data(&wrq[k]);
            else if ((ioctl_cmd == ES_DECRYPT) || (ioctl_cmd == ES_AUTO_DECRYPT))
                test_length = get_dec_input_data(&wrq[k]);

            if(test_length == 0) {
                ret = -1;
                printf("wrq %d test pattern length=0\n", k);
                goto exit;
            }

            wrq[k].data_length = test_length + wrq[k].IV_length;
        }

        if (ioctl(descript[k], ioctl_cmd, &wrq[k]) < 0) {
            printf("wrq %d fail\n", k);
            ret = -1;
            goto exit;
        }

        if(ioctl_cmd == ES_GETKEY) {
            printf("\nwrq %d Key =\n", k);
            for(i=0; i<(wrq[k].key_length/4); i++) {
                if(i >= 8)
                    break;
                printf("0x%08x\n", wrq[k].key_addr[i]);
            }

            printf("\nwrq %d IV =\n", k);
            for(i=0; i<(wrq[k].IV_length/4); i++) {
                if(i >= 4)
                    break;
                printf("0x%08x\n", wrq[k].IV_addr[i]);
            }
        }
        else {
            printf("\nwrq %d DataIn =\n", k);
            for (i = 0; i < 4; i++) {
                for (j = 0; j < 4; j++)
                    printf("0x%08x ", *(unsigned int *)(wrq[k].DataIn_addr + (i * 4 + j) * 4));
                printf("\n");
            }

            printf("\nwrq %d DataOut =\n", k);
            for (i = 0; i < 4; i++) {
                for (j = 0; j < 4; j++)
                    printf("0x%08x ", *(unsigned int *)(wrq[k].DataOut_addr + (i * 4 + j) * 4));
                printf("\n");
            }
        }
    }

exit:
    for(i=0; i<PROCESS_CNT; i++) {
        if(wrq[i].DataIn_addr)
            munmap((char*)wrq[i].DataIn_addr, dma_size[i]);

        if(descript[i])
            close(descript[i]);
    }

    return ret;
}
