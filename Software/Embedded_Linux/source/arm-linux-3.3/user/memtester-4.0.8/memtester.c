/*
 * memtester version 4
 *
 * Very simple but very effective user-space memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2007 Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Licensed under the terms of the GNU General Public License version 2 (only).
 * See the file COPYING for details.
 *
 */

#define __version__ "4.0.8"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include "types.h"
#include "sizes.h"
#include "tests.h"

#include <asm/ioctl.h>
#include <fcntl.h>

/* Function definitions. */
#include <frammap_if.h>

#define EXIT_FAIL_NONSTARTER    0x01
#define EXIT_FAIL_ADDRESSLINES  0x02
#define EXIT_FAIL_OTHERTEST     0x04

struct test tests[] = {
    { "Random Value", test_random_value },
    { "Compare XOR", test_xor_comparison },
    { "Compare SUB", test_sub_comparison },
    { "Compare MUL", test_mul_comparison },
    { "Compare DIV",test_div_comparison },
    { "Compare OR", test_or_comparison },
    { "Compare AND", test_and_comparison },
    { "Sequential Increment", test_seqinc_comparison },
    { "Solid Bits", test_solidbits_comparison },
    { "Block Sequential", test_blockseq_comparison },
    { "Checkerboard", test_checkerboard_comparison },
    { "Bit Spread", test_bitspread_comparison },
    { "Bit Flip", test_bitflip_comparison },
    { "Walking Ones", test_walkbits1_comparison },
    { "Walking Zeroes", test_walkbits0_comparison },
    { NULL, NULL }
};

#ifdef _SC_VERSION
void check_posix_system(void) {
    if (sysconf(_SC_VERSION) < 198808L) {
        fprintf(stderr, "A POSIX system is required.  Don't be surprised if "
            "this craps out.\n");
        fprintf(stderr, "_SC_VERSION is %lu\n", sysconf(_SC_VERSION));
    }
}
#else
#define check_posix_system()
#endif

#ifdef _SC_PAGE_SIZE
int memtester_pagesize(void) {
    int pagesize = sysconf(_SC_PAGE_SIZE);
    if (pagesize == -1) {
        perror("get page size failed");
        exit(EXIT_FAIL_NONSTARTER);
    }
    printf("pagesize is %ld\n", pagesize);
    return pagesize;
}
#else
int memtester_pagesize(void) {
    printf("sysconf(_SC_PAGE_SIZE) not supported; using pagesize of 8192\n");
    return 8192;
}
#endif

static void usage(void)
{
	printf("\nUsage: memtester [MB] [Frammap]\n");
	printf("1. memtester N   => will alloc from kernel and test N MB\n");
	printf("2. memtester N 0 => will alloc from DDR0 and test N MB\n");
	printf("3. memtester N 1 => will alloc from DDR1 and test N MB\n");
	printf("4. memtester N 2 => will alloc from DDR2 and test N MB\n");
}

int main(int argc, char **argv) {
    ul loops, loop, i;
    size_t pagesize, wantmb, wantbytes, wantbytes_orig, bufsize, halflen, count;
    ptrdiff_t pagesizemask;
    void volatile *buf, *aligned;
    ulv *bufa, *bufb;
    int do_mlock = 1, done_mem = 0;
    int exit_code = 0;
    int fbfd = -1, mode;
    frmmap_meminfo_t meminfo;
                    
    size_t maxbytes = -1; /* addressable memory, in bytes */
    size_t maxmb = (maxbytes >>20) + 1; /* addressable memory, in MB */

    printf("memtester version " __version__ " (%d-bit)\n", UL_LEN);
    printf("Copyright (C) 2007 Charles Cazabon.\n");
    printf("Licensed under the GNU General Public License version 2 (only).\n");
    printf("Harry version................. 1 \n");
    printf("\n");
    check_posix_system();
    pagesize = memtester_pagesize();
    pagesizemask = (ptrdiff_t) ~(pagesize - 1);
    printf("pagesizemask is 0x%tx\n", pagesizemask);

    if (argc < 2) {
        //fprintf(stderr, "need memory argument, in MB\n");
        usage();
        exit(EXIT_FAIL_NONSTARTER);
    }
    wantmb = (size_t) strtoul(argv[1], NULL, 0);
    if (wantmb > maxmb) {
	fprintf(stderr, "This system can only address %llu MB.\n", (ull) maxmb);
	exit(EXIT_FAIL_NONSTARTER);
    }
    wantbytes_orig = wantbytes = (size_t) (wantmb << 20);
    if (wantbytes < pagesize) {
        fprintf(stderr, "bytes < pagesize -- memory argument too large?\n");
        exit(EXIT_FAIL_NONSTARTER);
    }

    if (argc < 4) {
        loops = 0;
    } else {
        loops = strtoul(argv[3], NULL, 0);
    }

    printf("want %lluMB (%llu bytes)\n", (ull) wantmb, (ull) wantbytes);
    buf = NULL;

    while (!done_mem) {
        while (!buf && wantbytes) {

            if (argc >= 3) {
                mode = strtoul(argv[2], NULL, 0);
                
                if(mode == 0)
                    fbfd = open("/dev/frammap0", O_RDWR);
                else if(mode == 1)
                    fbfd = open("/dev/frammap1", O_RDWR);
                else if(mode == 2)
                    fbfd = open("/dev/frammap2", O_RDWR);
                else {
                    printf("open frammap%d fail\n", mode);
                    exit(exit_code);                    
                }

                if (fbfd == -1) {
                    printf("open frammap%d fail\n", mode);
                    exit(exit_code);
                } else {
                    printf("open frammap%d ok\n", mode);
                }
                
                if (ioctl(fbfd, FRM_IOGBUFINFO, &meminfo) < 0) {
                    printf("ioctl fail\n");
                    exit(exit_code);
                }
                meminfo.aval_bytes = wantbytes;
                buf = (unsigned char *)mmap(0, meminfo.aval_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
                if (buf == (unsigned char *)(-1)) {
                    printf("mmap fail\n");
                    exit(exit_code);
                }       
            } else {
                buf = (void volatile *) malloc(wantbytes);
                if (!buf) wantbytes -= pagesize;
            }
        }
        bufsize = wantbytes;
        printf("got  %lluMB (%llu bytes)", (ull) wantbytes >> 20,
            (ull) wantbytes);
        fflush(stdout);
        if (do_mlock) {
            printf(", trying mlock ...");
            fflush(stdout);
            if ((size_t) buf % pagesize) {
                /* printf("aligning to page -- was 0x%tx\n", buf); */
                aligned = (void volatile *) ((size_t) buf & pagesizemask) + pagesize;
                /* printf("  now 0x%tx -- lost %d bytes\n", aligned,
                 *      (size_t) aligned - (size_t) buf);
                 */
                bufsize -= ((size_t) aligned - (size_t) buf);
            } else {
                aligned = buf;
            }
            /* Try memlock */
            if (mlock((void *) aligned, bufsize) < 0) {
                switch(errno) {
                    case ENOMEM:
                        printf("too many pages, reducing...\n");
                        free((void *) buf);
                        buf = NULL;
                        wantbytes -= pagesize;
                        break;
                    case EPERM:
                        printf("insufficient permission.\n");
                        printf("Trying again, unlocked:\n");
                        do_mlock = 0;
                        free((void *) buf);
                        buf = NULL;
                        wantbytes = wantbytes_orig;
                        break;
                    default:
                        printf("failed for unknown reason.\n");
                        do_mlock = 0;
                        done_mem = 1;
                }
            } else {
                printf("locked.\n");
                done_mem = 1;
            }
        } else {
            done_mem = 1;
            printf("\n");
        }
    }

    if (!do_mlock) fprintf(stderr, "Continuing with unlocked memory; testing "
        "will be slower and less reliable.\n");

    halflen = bufsize / 2;
    count = halflen / sizeof(ul);
    bufa = (ulv *) aligned;
    bufb = (ulv *) ((size_t) aligned + halflen);

    /* loops: how many rounds */
    for(loop=1; ((!loops) || loop <= loops); loop++) {
        printf("Loop %lu", loop);
        if (loops) {
            printf("/%lu", loops);
        }
        printf(":\n");
        printf("  %-20s: ", "Stuck Address");
        fflush(stdout);
        if (!test_stuck_address(aligned, bufsize / sizeof(ul))) {
             printf("ok\n");
        } else {
            exit_code |= EXIT_FAIL_ADDRESSLINES;
        }
        for (i=0;;i++) {
            if (!tests[i].name) break;
            printf("  %-20s: ", tests[i].name);
            if (!tests[i].fp(bufa, bufb, count)) {
                printf("ok\n");
            } else {
                exit_code |= EXIT_FAIL_OTHERTEST;
            }
            fflush(stdout);
        }
        printf("\n");
        fflush(stdout);
    }
    if (do_mlock) munlock((void *) aligned, bufsize);
    printf("Done.\n");
    fflush(stdout);
    exit(exit_code);
}
