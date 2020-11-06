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

#include <common.h>
#include <asm/io.h>

#define DMA_LLP

#define SIZE_1MB (1 << 20)

#define EXIT_FAIL_NONSTARTER    0x01
#define EXIT_FAIL_ADDRESSLINES  0x02
#define EXIT_FAIL_OTHERTEST     0x04

typedef unsigned long ul;
typedef unsigned long long ull;
typedef unsigned long volatile ulv;

#define rand32() ((unsigned int) random32() | ( (unsigned int) random32() << 16))
#define rand_ul() rand32()
#define UL_ONEBITS 0xffffffff
#define UL_LEN 32
#define CHECKERBOARD1 0x55555555
#define CHECKERBOARD2 0xaaaaaaaa
#define UL_BYTE(x) ((x | x << 8 | x << 16 | x << 24))

char progress[] = "-\\|/";
#define PROGRESSLEN 4
#define PROGRESSOFTEN 2500

/* Function declaration. */

int test_stuck_address(unsigned long volatile *bufa, size_t count);
int test_random_value(unsigned long volatile *bufa, unsigned long volatile *bufb, size_t count);
int test_xor_comparison(unsigned long volatile *bufa, unsigned long volatile *bufb, size_t count);
int test_sub_comparison(unsigned long volatile *bufa, unsigned long volatile *bufb, size_t count);
int test_mul_comparison(unsigned long volatile *bufa, unsigned long volatile *bufb, size_t count);
int test_div_comparison(unsigned long volatile *bufa, unsigned long volatile *bufb, size_t count);
int test_or_comparison(unsigned long volatile *bufa, unsigned long volatile *bufb, size_t count);
int test_and_comparison(unsigned long volatile *bufa, unsigned long volatile *bufb, size_t count);
int test_seqinc_comparison(unsigned long volatile *bufa, unsigned long volatile *bufb, size_t count);
int test_solidbits_comparison(unsigned long volatile *bufa, unsigned long volatile *bufb, size_t count);
int test_checkerboard_comparison(unsigned long volatile *bufa, unsigned long volatile *bufb, size_t count);
int test_blockseq_comparison(unsigned long volatile *bufa, unsigned long volatile *bufb, size_t count);
int test_walkbits0_comparison(unsigned long volatile *bufa, unsigned long volatile *bufb, size_t count);
int test_walkbits1_comparison(unsigned long volatile *bufa, unsigned long volatile *bufb, size_t count);
int test_bitspread_comparison(unsigned long volatile *bufa, unsigned long volatile *bufb, size_t count);
int test_bitflip_comparison(unsigned long volatile *bufa, unsigned long volatile *bufb, size_t count);

static void dma_interference(ulv *dma_bufs, ulv *dma_bufd, size_t dma_xfer_len);

struct test
{
    char *name;
    int (*fp)(unsigned long volatile *bufa, unsigned long volatile *bufb, size_t count);
};

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

/* Function definitions. */

static inline ulong __seed(ulong x, ulong m)
{
	return (x < m) ? x + m : x;
}

ulong random32(void)
{
	ulong s1, s2, s3;
    ulong now = readl(CONFIG_TMR_BASE + 0);

#define LCG(x)	((x) * 69069)	/* super-duper LCG */
    s1 = __seed(LCG(now), 1);
    s2 = __seed(LCG(s1), 7);
    s3 = __seed(LCG(s2), 15);

#define TAUSWORTHE(s,a,b,c,d) ((s&c)<<d) ^ (((s <<a) ^ s)>>b)
    s1 = TAUSWORTHE(s1, 13, 19, 4294967294UL, 12);
    s2 = TAUSWORTHE(s2, 2, 25, 4294967288UL, 4);
    s3 = TAUSWORTHE(s3, 3, 11, 4294967280UL, 17);

	return (s1 ^ s2 ^ s3);
}

int compare_regions(ulv *bufa, ulv *bufb, size_t count) {
    int r = 0;
    size_t i;
    ulv *p1 = bufa;
    ulv *p2 = bufb;

    for (i = 0; i < count; i++, p1++, p2++) {
        if (*p1 != *p2) {
            printf("FAILURE: 0x%08lx != 0x%08lx at offset 0x%08lx.\n",
                (ul) *p1, (ul) *p2, (ul) i);
            /* printf("Skipping to next test..."); */
            r = -1;
        }
    }
    return r;
}

int test_stuck_address(ulv *bufa, size_t count) {
    ulv *p1 = bufa;
    unsigned int j;
    size_t i;

    printf("           ");
    for (j = 0; j < 16; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        printf("setting %3u", j);
        for (i = 0; i < count; i++) {
            *p1 = ((j + i) % 2) == 0 ? (ul) p1 : ~((ul) p1);
            *p1++;
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        p1 = (ulv *) bufa;
        for (i = 0; i < count; i++, p1++) {
            if (*p1 != (((j + i) % 2) == 0 ? (ul) p1 : ~((ul) p1))) {
                printf("FAILURE: possible bad address line at offset 0x%08lx.\n", (ul) i);
                printf("Skipping to next test...\n");
                return -1;
            }
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    return 0;
}

int test_random_value(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    ul j = 0;
    size_t i;

    putc(' ');
    for (i = 0; i < count; i++) {
        *p1++ = *p2++ = rand_ul();
        if (!(i % PROGRESSOFTEN)) {
            putc('\b');
            putc(progress[++j % PROGRESSLEN]);
        }
    }
    printf("\b \b");
    return compare_regions(bufa, bufb, count);
}

int test_xor_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ ^= q;
        *p2++ ^= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_sub_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ -= q;
        *p2++ -= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_mul_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ *= q;
        *p2++ *= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_div_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        if (!q) {
            q++;
        }
        *p1++ /= q;
        *p2++ /= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_or_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ |= q;
        *p2++ |= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_and_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ &= q;
        *p2++ &= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_seqinc_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ = *p2++ = (i + q);
    }
    return compare_regions(bufa, bufb, count);
}

int test_solidbits_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    ul q;
    size_t i;

    printf("           ");
    for (j = 0; j < 64; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        q = (j % 2) == 0 ? UL_ONEBITS : 0;
        printf("setting %3u", j);
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    return 0;
}

int test_checkerboard_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    ul q;
    size_t i;

    printf("           ");
    for (j = 0; j < 64; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        q = (j % 2) == 0 ? CHECKERBOARD1 : CHECKERBOARD2;
        printf("setting %3u", j);
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    return 0;
}

int test_blockseq_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    printf("           ");
    for (j = 0; j < 256; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        printf("setting %3u", j);
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (ul) UL_BYTE(j);
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    return 0;
}

int test_walkbits0_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    printf("           ");
    for (j = 0; j < UL_LEN * 2; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        printf("setting %3u", j);
        for (i = 0; i < count; i++) {
            if (j < UL_LEN) { /* Walk it up. */
                *p1++ = *p2++ = 0x00000001 << j;
            } else { /* Walk it back down. */
                *p1++ = *p2++ = 0x00000001 << (UL_LEN * 2 - j - 1);
            }
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    return 0;
}

int test_walkbits1_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    printf("           ");
    for (j = 0; j < UL_LEN * 2; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        printf("setting %3u", j);
        for (i = 0; i < count; i++) {
            if (j < UL_LEN) { /* Walk it up. */
                *p1++ = *p2++ = UL_ONEBITS ^ (0x00000001 << j);
            } else { /* Walk it back down. */
                *p1++ = *p2++ = UL_ONEBITS ^ (0x00000001 << (UL_LEN * 2 - j - 1));
            }
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    return 0;
}

int test_bitspread_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    printf("           ");
    for (j = 0; j < UL_LEN * 2; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        printf("setting %3u", j);
        for (i = 0; i < count; i++) {
            if (j < UL_LEN) { /* Walk it up. */
                *p1++ = *p2++ = (i % 2 == 0)
                    ? (0x00000001 << j) | (0x00000001 << (j + 2))
                    : UL_ONEBITS ^ ((0x00000001 << j)
                                    | (0x00000001 << (j + 2)));
            } else { /* Walk it back down. */
                *p1++ = *p2++ = (i % 2 == 0)
                    ? (0x00000001 << (UL_LEN * 2 - 1 - j)) | (0x00000001 << (UL_LEN * 2 + 1 - j))
                    : UL_ONEBITS ^ (0x00000001 << (UL_LEN * 2 - 1 - j)
                                    | (0x00000001 << (UL_LEN * 2 + 1 - j)));
            }
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    return 0;
}

int test_bitflip_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j, k;
    ul q;
    size_t i;

    printf("           ");
    for (k = 0; k < UL_LEN; k++) {
        q = 0x00000001 << k;
        for (j = 0; j < 8; j++) {
            printf("\b\b\b\b\b\b\b\b\b\b\b");
            q = ~q;
            printf("setting %3u", k * 8 + j);
            p1 = (ulv *) bufa;
            p2 = (ulv *) bufb;
            for (i = 0; i < count; i++) {
                *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
            }
            printf("\b\b\b\b\b\b\b\b\b\b\b");
            printf("testing %3u", k * 8 + j);
            if (compare_regions(bufa, bufb, count)) {
                return -1;
            }
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    return 0;
}

static int do_memtester(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    ul loops, loop, i;
    size_t wantmb, wantbytes, wantbytes_orig, bufsize, halflen, count;
    ulv *buf, *bufa, *bufb;
    ulv *dma_buf;
    int exit_code = 0;
    size_t maxbytes = PHYS_SDRAM_SIZE; /* addressable memory, in bytes */
    size_t maxmb = (maxbytes >> 20) + 1; /* addressable memory, in MB */
    size_t minmb = (SIZE_1MB >> 20) + 1; /* addressable memory, in MB */
    size_t dma_xfer_len;

    if (argc < 3)
        return CMD_RET_USAGE;

    wantmb = (size_t) simple_strtoul(argv[2], NULL, 10);
    if (wantmb > maxmb) {
        printf("This system can only address %llu MB.\n", (ull) maxmb);
        return CMD_RET_FAILURE;
    }
    if (wantmb < minmb) {
        printf("This system require memory space at least %llu MB.\n", (ull) minmb);
        return CMD_RET_FAILURE;
    }
    wantbytes_orig = wantbytes = (size_t) (wantmb << 20);
    bufsize = wantbytes - SIZE_1MB;
    dma_xfer_len = SIZE_1MB / 2;

    if (argc == 3) {
        loops = 0;
    } else {
        loops = simple_strtoul(argv[3], NULL, 10);;
    }

    printf("want %lluMB (%llu bytes)\n", (ull) wantmb, (ull) wantbytes);
    buf = (ulong *) simple_strtoul(argv[1], NULL, 16);
    dma_buf = (ulv *) ((size_t) buf + bufsize);

    halflen = bufsize / 2;
    count = halflen / sizeof(ul);
    bufa = (ulv *) buf;
    bufb = (ulv *) ((size_t) buf + halflen);

    dma_interference(dma_buf, (ulv *) ((size_t) dma_buf + dma_xfer_len), dma_xfer_len);

    for (loop=1; ((!loops) || loop <= loops); loop++) {
        printf("Loop %lu", loop);
        if (loops) {
            printf("/%lu", loops);
        }
        printf(":\n");
        printf("  %-20s: ", "Stuck Address");
        if (!test_stuck_address(buf, bufsize / sizeof(ul))) {
             printf("ok\n");
        } else {
            exit_code |= EXIT_FAIL_ADDRESSLINES;
        }
		if (ctrlc()) {
			putc ('\n');
			goto _exit;
		}
        for (i=0;;i++) {
            if (!tests[i].name) break;
            printf("  %-20s: ", tests[i].name);
            if (!tests[i].fp(bufa, bufb, count)) {
                printf("ok\n");
            } else {
                exit_code |= EXIT_FAIL_OTHERTEST;
            }
            if (ctrlc()) {
                putc ('\n');
                goto _exit;
            }
        }
        printf("\n");
    }

_exit:
    if (exit_code == 0)
        return CMD_RET_SUCCESS;
    else
        return CMD_RET_FAILURE;
}

U_BOOT_CMD(
	memtester,	4,	1,	do_memtester,
	"memory tester",
	"address number_of_megabytes [iterations]"
);

/*
 * dma test program
 */
#define FTDMAC020_OFFSET_ISR		0x0
#define FTDMAC020_OFFSET_TCISR		0x4
#define FTDMAC020_OFFSET_TCICR		0x8
#define FTDMAC020_OFFSET_EAISR		0xc
#define FTDMAC020_OFFSET_EAICR		0x10
#define FTDMAC020_OFFSET_TCRAW		0x14
#define FTDMAC020_OFFSET_EARAW		0x18
#define FTDMAC020_OFFSET_CH_ENABLED	0x1c
#define FTDMAC020_OFFSET_CH_BUSY	0x20
#define FTDMAC020_OFFSET_CR		    0x24
#define FTDMAC020_OFFSET_SYNC		0x28
#define FTDMAC020_OFFSET_REVISION	0x2c
#define FTDMAC020_OFFSET_FEATURE	0x30

#define FTDMAC020_OFFSET_CCR_CH(x)	(0x100 + (x) * 0x20)
#define FTDMAC020_OFFSET_CFG_CH(x)	(0x104 + (x) * 0x20)
#define FTDMAC020_OFFSET_SRC_CH(x)	(0x108 + (x) * 0x20)
#define FTDMAC020_OFFSET_DST_CH(x)	(0x10c + (x) * 0x20)
#define FTDMAC020_OFFSET_LLP_CH(x)	(0x110 + (x) * 0x20)
#define FTDMAC020_OFFSET_CYC_CH(x)	(0x114 + (x) * 0x20)

/*
 * Error/abort interrupt status/clear register
 * Error/abort status register
 */
#define FTDMAC020_EA_ERR_CH(x)		(1 << (x))
#define FTDMAC020_EA_ABT_CH(x)		(1 << ((x) + 16))

/*
 * Main configuration status register
 */
#define FTDMAC020_CR_ENABLE		(1 << 0)
#define FTDMAC020_CR_M0_BE		(1 << 1)	/* master 0 big endian */
#define FTDMAC020_CR_M1_BE		(1 << 2)	/* master 1 big endian */

/*
 * Channel control register
 */
#define FTDMAC020_CCR_ENABLE		(1 << 0)
#define FTDMAC020_CCR_DST_M1		(1 << 1)
#define FTDMAC020_CCR_SRC_M1		(1 << 2)
#define FTDMAC020_CCR_DST_INC		(0x0 << 3)
#define FTDMAC020_CCR_DST_DEC		(0x1 << 3)
#define FTDMAC020_CCR_DST_FIXED		(0x2 << 3)
#define FTDMAC020_CCR_SRC_INC		(0x0 << 5)
#define FTDMAC020_CCR_SRC_DEC		(0x1 << 5)
#define FTDMAC020_CCR_SRC_FIXED		(0x2 << 5)
#define FTDMAC020_CCR_HANDSHAKE		(1 << 7)
#define FTDMAC020_CCR_DST_WIDTH_8	(0x0 << 8)
#define FTDMAC020_CCR_DST_WIDTH_16	(0x1 << 8)
#define FTDMAC020_CCR_DST_WIDTH_32	(0x2 << 8)
#define FTDMAC020_CCR_DST_WIDTH_64	(0x3 << 8)
#define FTDMAC020_CCR_SRC_WIDTH_8	(0x0 << 11)
#define FTDMAC020_CCR_SRC_WIDTH_16	(0x1 << 11)
#define FTDMAC020_CCR_SRC_WIDTH_32	(0x2 << 11)
#define FTDMAC020_CCR_SRC_WIDTH_64	(0x3 << 11)
#define FTDMAC020_CCR_ABORT		    (1 << 15)
#define FTDMAC020_CCR_BURST_1		(0x0 << 16)
#define FTDMAC020_CCR_BURST_4		(0x1 << 16)
#define FTDMAC020_CCR_BURST_8		(0x2 << 16)
#define FTDMAC020_CCR_BURST_16		(0x3 << 16)
#define FTDMAC020_CCR_BURST_32		(0x4 << 16)
#define FTDMAC020_CCR_BURST_64		(0x5 << 16)
#define FTDMAC020_CCR_BURST_128		(0x6 << 16)
#define FTDMAC020_CCR_BURST_256		(0x7 << 16)
#define FTDMAC020_CCR_PRIVILEGED	(1 << 19)
#define FTDMAC020_CCR_BUFFERABLE	(1 << 20)
#define FTDMAC020_CCR_CACHEABLE		(1 << 21)
#define FTDMAC020_CCR_PRIO_0		(0x0 << 22)
#define FTDMAC020_CCR_PRIO_1		(0x1 << 22)
#define FTDMAC020_CCR_PRIO_2		(0x2 << 22)
#define FTDMAC020_CCR_PRIO_3		(0x3 << 22)
#define FTDMAC020_CCR_FIFOTH_1		(0x0 << 24)
#define FTDMAC020_CCR_FIFOTH_2		(0x1 << 24)
#define FTDMAC020_CCR_FIFOTH_4		(0x2 << 24)
#define FTDMAC020_CCR_FIFOTH_8		(0x3 << 24)
#define FTDMAC020_CCR_FIFOTH_16		(0x4 << 24)
#define FTDMAC020_CCR_MASK_TC		(1 << 31)

/*
 * Channel configuration register
 */
#define FTDMAC020_CFG_MASK_TCI		(1 << 0)	/* mask tc interrupt */
#define FTDMAC020_CFG_MASK_EI		(1 << 1)	/* mask error interrupt */
#define FTDMAC020_CFG_MASK_AI		(1 << 2)	/* mask abort interrupt */
#define FTDMAC020_CFG_SRC_HANDSHAKE(x)	(((x) & 0xf) << 3)
#define FTDMAC020_CFG_SRC_HANDSHAKE_EN	(1 << 7)
#define FTDMAC020_CFG_BUSY		(1 << 8)
#define FTDMAC020_CFG_DST_HANDSHAKE(x)	(((x) & 0xf) << 9)
#define FTDMAC020_CFG_DST_HANDSHAKE_EN	(1 << 13)
#define FTDMAC020_CFG_LLP_CNT(cfg)	(((cfg) >> 16) & 0xf)

/*
 * Link list descriptor pointer
 */
#define FTDMAC020_LLP_M1		(1 << 0)
#define FTDMAC020_LLP_ADDR(a)		((a) & ~0x3)

/*
 * Transfer size register
 */
#define FTDMAC020_CYC_MASK		0x3fffff

#define FTDMAC020_LLD_CTRL_DST_M1	(1 << 16)
#define FTDMAC020_LLD_CTRL_SRC_M1	(1 << 17)
#define FTDMAC020_LLD_CTRL_DST_INC	(0x0 << 18)
#define FTDMAC020_LLD_CTRL_DST_DEC	(0x1 << 18)
#define FTDMAC020_LLD_CTRL_DST_FIXED	(0x2 << 18)
#define FTDMAC020_LLD_CTRL_SRC_INC	(0x0 << 20)
#define FTDMAC020_LLD_CTRL_SRC_DEC	(0x1 << 20)
#define FTDMAC020_LLD_CTRL_SRC_FIXED	(0x2 << 20)
#define FTDMAC020_LLD_CTRL_DST_WIDTH_8	(0x0 << 22)
#define FTDMAC020_LLD_CTRL_DST_WIDTH_16	(0x1 << 22)
#define FTDMAC020_LLD_CTRL_DST_WIDTH_32	(0x2 << 22)
#define FTDMAC020_LLD_CTRL_DST_WIDTH_64	(0x3 << 22)
#define FTDMAC020_LLD_CTRL_SRC_WIDTH_8	(0x0 << 25)
#define FTDMAC020_LLD_CTRL_SRC_WIDTH_16	(0x1 << 25)
#define FTDMAC020_LLD_CTRL_SRC_WIDTH_32	(0x2 << 25)
#define FTDMAC020_LLD_CTRL_SRC_WIDTH_64	(0x3 << 25)
#define FTDMAC020_LLD_CTRL_MASK_TC	(1 << 28)
#define FTDMAC020_LLD_CTRL_FIFOTH_1	(0x0 << 29)
#define FTDMAC020_LLD_CTRL_FIFOTH_2	(0x1 << 29)
#define FTDMAC020_LLD_CTRL_FIFOTH_4	(0x2 << 29)
#define FTDMAC020_LLD_CTRL_FIFOTH_8	(0x3 << 29)
#define FTDMAC020_LLD_CTRL_FIFOTH_16	(0x4 << 29)

#define FTDMAC020_LLD_CYCLE_MASK	0x3fffff

struct ftdmac020_lld {
    uint src;       /* SrcAddr */
    uint dst;       /* DstAddr */
    uint next;      /* LLP */
    uint ctrl;      /* Control */
    uint cycle;     /* Total Size */
};
struct ftdmac020_lld lld;

struct ftdmac020_desc {
	uint ccr;       /* Cn_CSR */
	uint cfg;       /* Cn_CFG */
	uint src;       /* Cn_SrcAddr */
	uint dst;       /* Cn_DstAddr */
	uint next;      /* Cn_LLP */
	uint cycle;     /* Cn_SIZE */
};
struct ftdmac020_desc desc;

static void dma_interference(ulv *dma_bufs, ulv *dma_bufd, size_t dma_xfer_len)
{
    void __iomem *base = (void __iomem *) CONFIG_DMAC_BASE;
    uint ccr = 0, lld_ctrl = 0, cycle, chan_id = 0;
#if !defined(DMA_LLP)
    uint status = 0;
#endif
    uint i;

    ccr = FTDMAC020_CCR_ENABLE
        | FTDMAC020_CCR_BURST_8
        | FTDMAC020_CCR_PRIO_0
        | FTDMAC020_CCR_FIFOTH_1;
    lld_ctrl = FTDMAC020_LLD_CTRL_FIFOTH_1;

    ccr |= FTDMAC020_CCR_SRC_WIDTH_32;
    lld_ctrl |= FTDMAC020_LLD_CTRL_SRC_WIDTH_32;

    ccr |= FTDMAC020_CCR_DST_WIDTH_32;
    lld_ctrl |= FTDMAC020_LLD_CTRL_DST_WIDTH_32;

    ccr |= FTDMAC020_CCR_SRC_INC;
    lld_ctrl |= FTDMAC020_LLD_CTRL_SRC_INC;

    ccr |= FTDMAC020_CCR_DST_INC;
    lld_ctrl |= FTDMAC020_LLD_CTRL_DST_INC;

    ccr |= FTDMAC020_CCR_SRC_M1;  /* SRC_SEL */
    ccr |= FTDMAC020_CCR_DST_M1;  /* DST_SEL */
    lld_ctrl |= FTDMAC020_LLD_CTRL_SRC_M1;  /* SRC_SEL */
    lld_ctrl |= FTDMAC020_LLD_CTRL_DST_M1;  /* DST_SEL */

    cycle = dma_xfer_len / 4; //src width is 32-bit

    desc.src = lld.src = (uint) dma_bufs;
    desc.dst = lld.dst = (uint) dma_bufd;
    printf("dma src 0x%x, dst 0x%x\n", desc.src, desc.dst);

    desc.next  = 0;
    desc.cycle = cycle & FTDMAC020_CYC_MASK;
    desc.ccr   = ccr;
    desc.cfg   = 0;

    lld.next  = 0;
    lld.cycle = cycle & FTDMAC020_LLD_CYCLE_MASK;
    lld.ctrl  = lld_ctrl;

#if defined(DMA_LLP)
    /* LLP setting */
    desc.ccr |= FTDMAC020_CCR_MASK_TC;
    lld.ctrl |= FTDMAC020_LLD_CTRL_MASK_TC;
    desc.next = FTDMAC020_LLP_ADDR((uint) &lld) | FTDMAC020_LLP_M1;
    lld.next = FTDMAC020_LLP_ADDR((uint) &lld) | FTDMAC020_LLP_M1;
#endif

    for (i = 0; i < cycle; i++)
        *(dma_bufs + i) = i;

    /* clear interrupt status */
    writel(readl(base + FTDMAC020_OFFSET_TCISR), base + FTDMAC020_OFFSET_TCICR);

    /* enable dma controller */
    writel(FTDMAC020_CR_ENABLE, base + FTDMAC020_OFFSET_CR);

    /* start chain */
    writel(desc.src, base + FTDMAC020_OFFSET_SRC_CH(chan_id));
    writel(desc.dst, base + FTDMAC020_OFFSET_DST_CH(chan_id));
    writel(desc.next, base + FTDMAC020_OFFSET_LLP_CH(chan_id));
    writel(desc.cycle, base + FTDMAC020_OFFSET_CYC_CH(chan_id));

    /* go */
    writel(desc.cfg, base + FTDMAC020_OFFSET_CFG_CH(chan_id));
    writel(desc.ccr, base + FTDMAC020_OFFSET_CCR_CH(chan_id));

#if !defined(DMA_LLP)
    /* check dma done */
    printf("Dma start...");
    do {
        status = readl(base + FTDMAC020_OFFSET_TCISR);
    } while ((status & (1 << chan_id)) == 0);
    writel(1 << chan_id, base + FTDMAC020_OFFSET_TCICR);
    printf("done\n");

    for (i = 0; i < cycle; i++)
        if (*(dma_bufd + i) != i)
            printf("data error, 0x%x = 0x%x (golden 0x%x)\n",
                    (uint) (dma_bufd + i), (uint) *(dma_bufd + i), (uint) i);
#endif
}

#if 0
static int do_dmatester(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    ul loops;
    size_t wantmb, wantbytes, bufsize;
    ulv *buf, *dma_buf;
    size_t maxbytes = PHYS_SDRAM_SIZE; /* addressable memory, in bytes */
    size_t maxmb = (maxbytes >> 20) + 1; /* addressable memory, in MB */
    size_t minmb = (SIZE_1MB >> 20) + 1; /* addressable memory, in MB */
    size_t dma_xfer_len;

    if (argc < 3)
        return CMD_RET_USAGE;

    wantmb = (size_t) simple_strtoul(argv[2], NULL, 10);
    if (wantmb > maxmb) {
        printf("This system can only address %llu MB.\n", (ull) maxmb);
        return CMD_RET_FAILURE;
    }
    if (wantmb < minmb) {
        printf("This system require memory space at least %llu MB.\n", (ull) minmb);
        return CMD_RET_FAILURE;
    }
    wantbytes = (size_t) (wantmb << 20);
    bufsize = wantbytes - SIZE_1MB;
    dma_xfer_len = SIZE_1MB / 2;

    if (argc == 3) {
        loops = 0;
    } else {
        loops = simple_strtoul(argv[3], NULL, 10);;
    }

    printf("want %lluMB (%llu bytes)\n", (ull) wantmb, (ull) wantbytes);
    buf = (ulong *) simple_strtoul(argv[1], NULL, 16);
    dma_buf = (ulv *) ((size_t) buf + bufsize);
    //dma_bufs = dma_buf;
    //dma_bufd = (ulv *) ((size_t) dma_buf + dma_xfer_len);

    dma_interference(dma_buf, (ulv *) ((size_t) dma_buf + dma_xfer_len), dma_xfer_len);

    return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	dmatester,	4,	1,	do_dmatester,
	"dma tester",
	"address number_of_megabytes [iterations]"
);
#endif
