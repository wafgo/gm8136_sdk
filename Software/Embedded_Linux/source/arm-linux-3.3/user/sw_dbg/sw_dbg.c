#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include<termios.h>

#define _GNU_SOURCE
#include <getopt.h>

#include "config.h"

#define MAX_PAGES 32
#define BUF_SIZE 0x40000
#define PAGE_HNUM (4)
#define PAGE_VNUM (16)

static int x=-1, y=-1;

static 	int fd_idx = 0;
static int fd[MAX_PAGES]    = {-1};
static void* mmapbase[MAX_PAGES] = {0};
static uint32_t fd_bitmap    = 0;
static int base[MAX_PAGES]  = {0};
static int pbase[MAX_PAGES] = {0};
static int memoffset[MAX_PAGES] = {0};
static int psize[MAX_PAGES] = {0};
static int vprintk = -1;
static char getch(void)
{ 
	int i;
	char ch; 
	struct termios _old, _new; 

	tcgetattr (0, &_old); 
	memcpy (&_new, &_old, sizeof (struct termios)); 
	_new.c_lflag &= ~(ICANON | ECHO); 
	_new.c_cc[VTIME]=0;
	_new.c_cc[VMIN]=1;
	tcsetattr (0, TCSANOW, &_new); 
	i = read (0, &ch, 1); 
	tcsetattr (0, TCSANOW, &_old); 
	if (i == 1)/* ch is the character to use */   
		return ch; 
	else/* there was some problem; complain, return error, whatever */   
		printf("error!n");; 
	return 0;
}

#define KEY_UP    0x415b1b
#define KEY_DOWN  0x425b1b
#define KEY_LEFT  0x445b1b
#define KEY_RIGHT 0x435b1b
#define KEY_ESC   0x1b1b
#define KEY_ENTER 10
#define KEY_TAB   9

union key{
	int value;
	char oneByte[3];
};

static int get_input(void)
{
	union key ch = {0};

	ch.oneByte[0] = getch();
	if(ch.oneByte[0] == 0x1b) {
		ch.oneByte[1] = getch();
		if(ch.oneByte[1] == 0x5b) {
			ch.oneByte[2] = getch();
		}
	}
	return ch.value;
}

static void corse_move(int act, int num)
{
	int i;
	for(i=0;i<num;i++){
		printf("%c%c%c",(act)&0xff,(act>>0x8)&0xff,(act>>16)&0xff);
	}
	fflush(stdout);
}

static int get_value(void *data, int size)
{
	int pos = 0;
	unsigned int value=0;
	int ch;
	int ret = 0;
	unsigned char *tdata  =data; 

	if(size>4)
		size = 4;

	while(1) {
		ch = get_input();
		switch(ch) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if(pos<size*2) {
				pos++;
				if(pos) {
					value<<=4;
				}
				value |= (ch-'0');
				printf("%c", ch);fflush(stdout);
			}
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			if(pos<size*2) {
				pos++;
				if(pos) {
					value<<=4;
				}
				value |= (ch-'a'+10);
				printf("%c", ch);fflush(stdout);
			}
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			if(pos<size*2) {
				pos++;
				if(pos) {
					value<<=4;
				}
				value |= (ch-'A'+10);
				printf("%c", ch-'A'+'a');fflush(stdout);
			}
			break;
		case 0x08:				/* ^H  - backspace */
			if(pos>0) {
				pos--;
				value >>= 4;
#if 1
				corse_move(KEY_LEFT, 1);
				printf("%c",' ');fflush(stdout);
				corse_move(KEY_LEFT, 1);
#else
				printf("%c",ch);fflush(stdout);
				printf("%c",' ');fflush(stdout);
				printf("%c",ch);fflush(stdout);
#endif
			}
			break;
		case 'q':
		case 'Q':
		case KEY_ESC:
			ret = -1;
			goto end;
		case KEY_ENTER:
			if(pos == 0) {
				ret = -1;
			}
			goto end;
		}
	}
end:
	corse_move(KEY_LEFT, pos);
	for(ch=0;ch<size;ch++) {
		tdata[ch] = (value>>(8*ch))&0xff;
	}
	return ret;
}

static int get_array(uint8_t *data,uint8_t size)
{
	int pos = 0;
	uint8_t value=0;
	int ch;
	int ret = 0;

	for(ch=0;ch<size;ch++) {
	  printf("00");
	}
	corse_move(KEY_LEFT, size*2);

	while(1) {
		ch = get_input();
		switch(ch) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if(pos<size*2) {
				value = data[pos>>1];
				if(pos&0x01){
					value &=0xf0;
					value |= (ch-'0');
				}
				else{
					value &=0x0f;
					value |= ((ch-'0')<<4);
				}
				data[pos>>1]=value;
				printf("%c", ch);fflush(stdout);				
				pos++;
			}
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			if(pos<size*2) {
				value = data[pos>>1];
				if(pos&0x01){
					value &=0xf0;
					value |= (ch-'a'+10);
				}
				else{
					value &=0x0f;
					value |= ((ch-'a'+10)<<4);
				}
				data[pos>>1]=value;
				printf("%c", ch);fflush(stdout);				
				pos++;
			}
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			if(pos<size*2) {
				value = data[pos>>1];
				if(pos&0x01){
					value &=0xf0;
					value |= (ch-'A'+10);
				}
				else{
					value &=0x0f;
					value |= ((ch-'A'+10)<<4);
				}
				data[pos>>1]=value;
				printf("%c", ch-'A'+'a');fflush(stdout);
				pos++;
			}
			break;
		case KEY_LEFT:
			if(pos>0) {
				pos--;
				corse_move(KEY_LEFT, 1);
			}
			break;
		case KEY_RIGHT:
			if(pos<size*2) {
				pos++;
				corse_move(KEY_RIGHT, 1);
			}
			break;
		case 'q':
		case 'Q':
		case KEY_ESC:
			ret = -1;
			goto end;
		case KEY_ENTER:
			if(pos == 0) {
				ret = -1;
			}
			goto end;
		}
	}
end:
	corse_move(KEY_LEFT, pos);
	return ret;
}

static void modify_value(int offset, int idx)
{
	unsigned int value;
	int pos = 0;
	int ch;
	uint32_t *pmembase = mmapbase[idx]+memoffset[idx]+offset;
	
	value = *pmembase;

	printf("\033[7m%08x\033[0m",value);
	fflush(stdout);
	corse_move(KEY_LEFT, 8);
	fflush(stdout);

	while(1) {
		ch = get_input();
		switch(ch) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if(pos<=7) {
				value&=~(0xf<<((7-pos)<<2));
				value |= ((ch-'0')<<((7-pos)<<2));
				printf("\033[7m%c\033[0m", ch);fflush(stdout);
				pos++;
			}
			if(pos==8) {
				pos--;
				corse_move(KEY_LEFT,1);
			}				
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			if(pos<=7) {
				value&=~(0xf<<((7-pos)<<2));
				value |= ((ch-'a'+10)<<((7-pos)<<2));
				printf("\033[7m%c\033[0m", ch);fflush(stdout);
				pos++;
			}
			if(pos==8) {
				pos--;
				corse_move(KEY_LEFT,1);
			}
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			if(pos<=7) {
				value&=~(0xf<<((7-pos)<<2));
				value |= ((ch-'A'+10)<<((7-pos)<<2));
				printf("\033[7m%c\033[0m", ch-'A'+'a');fflush(stdout);
				pos++;
			}
			if(pos==8) {
				pos--;
				corse_move(KEY_LEFT,1);
			}
			break;
		case KEY_RIGHT:
			if(pos<7) {
				pos++;
				corse_move(KEY_RIGHT,1);
			}
			break;
		case KEY_LEFT:
			if(pos>0) {
				pos--;
				corse_move(KEY_LEFT,1);
			}
			break;
		case KEY_ENTER:
			goto out;
		case 'q':
		case 'Q':
		case KEY_ESC:
			goto quit;
		}
	}
out:
	*pmembase = value;
quit:
	value = *pmembase;
	corse_move(KEY_LEFT, pos);
	printf("%08x", value);
	fflush(stdout);
	corse_move(KEY_LEFT, 8);
}

static int get_and_set_free_index(void)
{
	int ret = -1;
	int i;

	for(i=0;i<32;i++)
		if(!((fd_bitmap>>i)&0x01))
			break;

	if(i<32) {
		fd_bitmap |= 1<<i;
		ret = i;
	}
	return ret;
}

#define PAGE_SIZE 0x1000

static int create_page(unsigned int baddr, int size)
{
	int idx;
	int ret = 0;
	unsigned int tmp;
	
	idx = get_and_set_free_index();

	fd[idx] = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd[idx] < 0) {
		printf("Open /dev/mem fail!\n");
		return -1;
	}
	
	tmp = baddr&(PAGE_SIZE-1);

	if(tmp) {
		baddr &= ~(PAGE_SIZE-1);
		size += tmp;
	}
	
	size = (size + (PAGE_SIZE-1))&(~(PAGE_SIZE-1));
	mmapbase[idx] = mmap(NULL, size, (PROT_READ | PROT_WRITE), MAP_SHARED,
			fd[idx], baddr & ~(off_t)(PAGE_SIZE - 1));

	if(mmapbase[idx]!=MAP_FAILED) {
		memoffset[idx] = baddr - (baddr & ~(PAGE_SIZE - 1));
		pbase[idx] = baddr;
		psize[idx] = size;		
		ret = idx;		
	}
	else {
		printf("0x%08x mmap fail!\n",baddr);
	}
	return ret;
}

static void free_page(int idx)
{
	munmap(mmapbase[idx], psize[idx]);
	close(fd[idx]);
	mmapbase[idx] = 0;
	pbase[idx] = 0;
	psize[idx] = 0;
	fd[idx] = -1;
	fd_bitmap &= ~(1<<idx);
	base[idx] = 0;
	memoffset[idx] = 0;
	return;
}

static void updata_screan(uint32_t base, int offset, int idx, int b_reset_cur)
{
	int i;
	char t_cmd[200];
	uint32_t pbuf[PAGE_HNUM*PAGE_VNUM];

	memcpy(pbuf, mmapbase[idx]+memoffset[idx]+offset*16*16, PAGE_HNUM*PAGE_VNUM*4);

	strcpy(t_cmd, "clear");
	system(t_cmd);
	
	printf("0x%08x  0x%02x %3s 0x%02x %3s 0x%02x %3s 0x%02x\n", base, 0, " ", 4, " ", 8 , " ", 12);
	printf("%s-%s-%s-%s-%s\n","--------","--------","--------","--------","--------");
	for(i=0; i<PAGE_VNUM; i++)
		printf(" 0x%05x| %08x %08x %08x %08x\n", 
		       (offset*16+i)*16, pbuf[i*4], pbuf[i*4+1], pbuf[i*4+2], pbuf[i*4+3]);
	fflush(stdout);

	if(b_reset_cur) {
		x=0;
		y=0;
	}

	corse_move(KEY_RIGHT, 10+x*9);
	corse_move(KEY_UP, 16-y);
}

static void help_info(void)
{
	char *t_cmd = "clear";
	system(t_cmd);
	printf("Help Menu\n");
	printf("===========================\n");
	printf("p:       Previous Page!\n");
	printf("n:       Next Page!\n");
	printf("w/Enter: Modify value!\n");
	printf("u:       Refresh Memory!\n");
	printf("TAB:     Switch Memory!\n");
	printf("c:       Create new memory mapping!\n");
	printf("k:       Kill current memory mapping!\n");
	printf("f:       Fill pattern to memory!\n");
	printf("s:       Save data to file!\n");
	printf("q/ESC  :   Exit!\n");
	while(1){
		int ch;		
		ch = get_input();
		switch(ch){
		case 'h':
		case 'H':
		case 'q':
		case 'Q':
		case KEY_ESC:
			return;
		}
	}
}

static void save2file(void)
{
	int fd = -1;
	void *mmapbase=0;	
	uint32_t offset;
	int woffset;
	uint32_t base;
	uint32_t size,wsize,map_size;
	uint8_t *pbuf = NULL;
	char fname[64];
	FILE *fsave=NULL;

	printf("phy address:0x");fflush(stdout);
	if(get_value(&base, 4)<0) {
		printf("\nThe input value is not correct!\n");
		printf("Please push u to reflush screan!\n");
		return;
	}
	printf("\n");fflush(stdout);
	printf("phy size:0x");fflush(stdout);
	if(get_value(&size, 4)<0) {
		printf("\nThe input value is not correct!\n");
		printf("Please push u to reflush screan!\n");
		return;
	}
	printf("\n");fflush(stdout);
	printf("file name:");fflush(stdout);
	scanf("%s", fname);

	fsave = fopen(fname, "w");
	if(fsave == NULL) {
		perror("fopen:");
		return;
	}
	
	pbuf = malloc(BUF_SIZE);
	if(!pbuf) {
		perror("fill_memory:(malloc):");
		goto err;
	}
	memset(pbuf, 0, BUF_SIZE);
	fd = open("/dev/mem", O_RDWR | O_SYNC);

	if (fd < 0) {
		printf("Open /dev/mem fail!\n");
		goto err;
	}

	map_size = (size+(PAGE_SIZE-1))& ~(PAGE_SIZE-1);

	mmapbase = mmap(NULL, map_size, (PROT_READ | PROT_WRITE), MAP_SHARED,
			 fd, base & ~(off_t)(PAGE_SIZE - 1));

	if(mmapbase==MAP_FAILED) {
		perror("[fill_memory]mmap:\n");
		goto err1;
	}

	offset = base - (base & ~(PAGE_SIZE - 1));
	woffset = 0;
	while(size) {
		if(size>BUF_SIZE)
			wsize = BUF_SIZE;
		else
			wsize = size;

		memcpy(pbuf, mmapbase+offset+woffset, wsize);

		fwrite(pbuf, wsize, 1, fsave);

		woffset += wsize;
		size -= wsize;
	}
	munmap(mmapbase, map_size);
err1:
	if(fsave)
		fclose(fsave);
	close(fd);
err:
	if(pbuf)
		free(pbuf);
	return;
}



static void fill_memory(void)
{
	int fd = -1;
	void *mmapbase=0;	
	uint32_t offset;
	int poffset;
	uint32_t base;
	uint32_t size,wsize,map_size;
	uint8_t pat_size;
	uint8_t *pat_buf = NULL;
	uint8_t *pbuf = NULL;
	int i;

	printf("phy address:0x");fflush(stdout);
	if(get_value(&base, 4)<0) {
		printf("\nThe input value is not correct!\n");
		printf("Please push u to reflush screan!\n");
		return;
	}
	printf("\n");fflush(stdout);
	printf("phy size:0x");fflush(stdout);
	if(get_value(&size, 4)<0) {
		printf("\nThe input value is not correct!\n");
		printf("Please push u to reflush screan!\n");
		return;
	}
	printf("\n");fflush(stdout);
	printf("pattern size:0x");fflush(stdout);
	if(get_value(&pat_size, 1)<0) {
		printf("\nThe input value is not correct!\n");
		printf("Please push u to reflush screan!\n");
		return;
	}
	
	pat_buf = malloc(pat_size);
	if(!pat_buf) {
		perror("fill_memory:(malloc):");
		return;
	}
	memset(pat_buf,0, pat_size);
	printf("\n");fflush(stdout);
	printf("pattern:0x");fflush(stdout);
	if(get_array((uint8_t*)pat_buf, pat_size)<0) {
		printf("\nThe input value is not correct!\n");
		printf("Please push u to reflush screan!\n");
		goto err;
	}

	if(pat_size>1) {
		for(i=0; i<pat_size/2;i++){
			pat_buf[i] ^= pat_buf[pat_size-i-1];
			pat_buf[pat_size-i-1] ^= pat_buf[i];
			pat_buf[i] ^= pat_buf[pat_size-i-1];
		}
	}

	pbuf = malloc(BUF_SIZE);
	if(!pbuf) {
		perror("fill_memory:(malloc):");
		goto err;
	}
	memset(pbuf, 0, BUF_SIZE);
	fd = open("/dev/mem", O_RDWR | O_SYNC);

	if (fd < 0) {
		printf("Open /dev/mem fail!\n");
		goto err;
	}

	map_size = (size+(PAGE_SIZE-1))& ~(PAGE_SIZE-1);

	mmapbase = mmap(NULL, map_size, (PROT_READ | PROT_WRITE), MAP_SHARED,
			 fd, base & ~(off_t)(PAGE_SIZE - 1));

	if(mmapbase==MAP_FAILED) {
		perror("[fill_memory]mmap:\n");
		goto err1;
	}

	offset = base - (base & ~(PAGE_SIZE - 1));
	poffset = 0;
	while(size) {
		uint8_t *tbuf;
		int poff;
		
		if(size>BUF_SIZE)
			wsize = BUF_SIZE;
		else
			wsize = size;
		
		tbuf = pbuf;

		if((poff = poffset%pat_size)!=0) {
			memcpy(tbuf, pat_buf+poff, pat_size-poff);
			tbuf+=(pat_size-poff);
		}
		poff = tbuf-pbuf;
		for(i=0; i<((wsize-poff)/pat_size); i++) {
			memcpy(tbuf, pat_buf, pat_size);
			tbuf+= pat_size;
		}
		
		if((poff=pbuf+wsize-tbuf)!=0) {
			memcpy(tbuf, pat_buf, poff);
		}

		memcpy(mmapbase+offset+poffset, pbuf, wsize);
		poffset += wsize;
		size -= wsize;
	}
	munmap(mmapbase, map_size);
err1:
	close(fd);
err:
	if(pat_buf)
		free(pat_buf);
	if(pbuf)
		free(pbuf);
	return;
}

static void dump_reg(uint32_t base, uint32_t size)
{
	int tfd = -1;
	void *mmapbase;	
	uint32_t map_size, wsize;
	int offset, woffset;
	uint32_t *pbuf;
	int i;
	unsigned int tmp;

	pbuf = malloc(BUF_SIZE);
	if(!pbuf) {
		perror("[dump_reg]malloc:");
		exit(1);
	}

	tfd = open("/dev/mem", O_RDWR | O_SYNC);

	if (tfd < 0) {
		perror("[dump_reg]Open(/dev/mem):");
		free(pbuf);
		exit(1);
	}

    tmp = base&(PAGE_SIZE-1);

    if(tmp) {
        base &= ~(PAGE_SIZE-1);
        size += tmp;
    }   

	map_size = (size+(PAGE_SIZE-1))& ~(PAGE_SIZE-1);
	mmapbase = mmap(NULL, map_size, (PROT_READ | PROT_WRITE), MAP_SHARED,
			tfd, base & ~(off_t)(PAGE_SIZE - 1));
	if(mmapbase==MAP_FAILED){
		perror("[dump_reg]mmap:");
		goto err;
	}

	offset = base - (base & ~(PAGE_SIZE - 1));

	printf("\t   0x%02x %3s 0x%02x %3s 0x%02x %3s 0x%02x\n", 0, " ", 4, " ", 8 , " ", 12);
	printf("%s-%s-%s-%s-%s\n","--------","--------","--------","--------","--------");
	woffset = 0;
	while(size) {
		if(size>BUF_SIZE)
			wsize = BUF_SIZE;
		else
			wsize = size;

		memcpy(pbuf, mmapbase+offset+woffset, wsize);

		for(i=0; i<(wsize>>4); i++)
			printf("0x%05x| %08x %08x %08x %08x\n", 
			       16*i+(woffset>>4), pbuf[i*4], pbuf[i*4+1], pbuf[i*4+2], pbuf[i*4+3]);
		woffset += wsize;
		size -= wsize;
	}
	fflush(stdout);
	free(pbuf);
	munmap(mmapbase, map_size);
err:
	close(tfd);
}

static int get_next_idx(int idx)
{
	int k;
	
	for(k=idx+1;k<MAX_PAGES;k++) {
		if(fd_bitmap&(1<<k))
			return k;
	}
	for(k=0;k<idx;k++) {
		if(fd_bitmap&(1<<k))
			return k;
	}
	return -1;
}

static void sig_int(void)
{
	char *t_cmd = "clear";
	int k;

	printf("Ctrl+C to end program\n");	

	for(k=0;k<MAX_PAGES;k++) {
		if(fd_bitmap&(1<<k))
			free_page(k);
	}
	system(t_cmd);

	if(vprintk>=0) {
		snprintf(t_cmd, 64, "echo %i > /proc/sys/kernel/printk",vprintk);
		system(t_cmd);
	}
	exit(0);
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int option_index;
	int k;
	char t_cmd[64];
	char t_file[32];
	char *pstr;
	static int bx=0, by=0, ex=(PAGE_HNUM-1), ey=(PAGE_VNUM-1);	
	unsigned int baddr = 0;
	unsigned int size = 0x1000;
	struct option longopts[] = {
		{"dump",   1, NULL, 'd'},
		{0,        0,    0,  0}
	};

	snprintf(t_file, 32, "%s.ini", argv[0]);
	ret = access(t_file, R_OK);
	pstr = strrchr(argv[0], '/');
	if(pstr)
		pstr++;
	else
		pstr = argv[0];
	
	if(ret) {
		snprintf(t_file, 32, "/etc/%s.ini", pstr);
		ret = access(t_file, R_OK);
	}
	if(ret) {
		snprintf(t_file, 32, "/%s.ini", pstr);
		ret = access(t_file, R_OK);
	}
	if(ret)
		goto parse;
	ret = getconfigstr("common", "module", t_cmd, 64, t_file);
	if(ret < 0)
		snprintf(t_cmd, 64, "%s", "common");
	
	ret = getconfiguint(t_cmd, "baseaddr", &baddr, t_file);
	ret = getconfiguint(t_cmd, "size", &size, t_file);
	parse:
	while ((k = getopt_long(argc, argv, "d:", longopts, &option_index)) != EOF){
		switch (k) {
		case 'd':
			baddr = strtoul(optarg,NULL,0);
			dump_reg(baddr, 0x1000);
			exit(0);
			break;
		default:			
			break;
		}
	}

	if(signal(SIGINT,(void *)sig_int)==SIG_ERR) {
		printf("signal ctrl+c error\n");
		ret = -1;
		goto end;
	}

	{
		int proc_fd;
		proc_fd = open("/proc/sys/kernel/printk", O_RDWR);
		read(proc_fd, t_cmd, 32);
		t_cmd[3] = '\0';
		vprintk = strtol(t_cmd, NULL, 0);
		close(proc_fd);
		snprintf(t_cmd, 64, "echo 0 > /proc/sys/kernel/printk");
		system(t_cmd);
	}


	printf("Default base=0x%x, size=0x%x\n",baddr, size);
	
	fd_idx = create_page(baddr, size);

	if(fd_idx < 0) {
		ret = -1;
		goto end;
	}

	strcpy(t_cmd, "clear");
	system(t_cmd);
	updata_screan(pbase[fd_idx], base[fd_idx], fd_idx, 1);
	while(1){
		int ch;
		ch = get_input();
		switch(ch) {
		case KEY_UP:
			if(y>by) {
				y--;
				corse_move(KEY_UP,1);
			}
			break;
		case KEY_DOWN:
			if(y<ey) {
				y++;
				corse_move(KEY_DOWN,1);
			}
			break;
		case KEY_RIGHT:
			if(x<ex) {
				x++;
				corse_move(KEY_RIGHT,9);
			}
			break;
		case KEY_LEFT:
			if(x>bx) {
				x--;
				corse_move(KEY_LEFT,9);
			}
			break;
		case KEY_TAB:
			k = get_next_idx(fd_idx);
			if(k>=0) {
				fd_idx = k;
				updata_screan(pbase[fd_idx], base[fd_idx], fd_idx, 1);
			}			
			break;
		case 'k':
		case 'K':
			k = get_next_idx(fd_idx);
			if(k>=0) {
				free_page(fd_idx);
				fd_idx = k;
			}
			updata_screan(pbase[fd_idx], base[fd_idx], fd_idx, 1);
			break;
		case 'c':
			strcpy(t_cmd, "clear");
			system(t_cmd);
			printf("phy address:0x");fflush(stdout);
			if(get_value(&baddr, 4)<0) {
				printf("\nThe input value is not correct!\n");
				printf("Please push u to reflush screan!\n");
				break;
			}
			printf("\n");fflush(stdout);
			printf("phy size:0x");fflush(stdout);
			if(get_value(&size, 4)<0) {
				printf("\nThe input value is not correct!\n");
				printf("Please push u to reflush screan!\n");
				break;
			}
			k = create_page(baddr, size);
			if(k>=0)
				fd_idx = k;
			updata_screan(pbase[fd_idx], base[fd_idx], fd_idx, 1);
			break;
		case 'u':
		case 'U':
			updata_screan(pbase[fd_idx], base[fd_idx], fd_idx, 0);
			break;
		case KEY_ENTER:
		case 'w':
		case 'W':
			modify_value((base[fd_idx]*16+y)*16+x*4 , fd_idx);
			break;
		case 'n':
		case 'N':
			if((base[fd_idx]+1)*(PAGE_VNUM*PAGE_HNUM) < (psize[fd_idx]>>2)) {
				base[fd_idx] ++;
				updata_screan(pbase[fd_idx], base[fd_idx], fd_idx, 1);
			}
			break;
		case 'p':
		case 'P':
			if(base[fd_idx] >0) {
				base[fd_idx]--;
				updata_screan(pbase[fd_idx], base[fd_idx], fd_idx, 1);
			}
			break;
		case 'f':
		case 'F':
			strcpy(t_cmd, "clear");
			system(t_cmd);
			fill_memory();
			updata_screan(pbase[fd_idx], base[fd_idx], fd_idx, 0);
			break;
		case 's':
		case 'S':
			strcpy(t_cmd, "clear");
			system(t_cmd);
			save2file();
			updata_screan(pbase[fd_idx], base[fd_idx], fd_idx, 0);
			break;
		case 'h':
		case 'H':
			help_info();
			updata_screan(pbase[fd_idx], base[fd_idx], fd_idx, 0);
			break;
		case 'q':
		case 'Q':
		case KEY_ESC:
			goto end;
		}
	}
end:
	if(!ret) {
		strcpy(t_cmd, "clear");
		system(t_cmd);
	}
	if(vprintk>=0) {
		snprintf(t_cmd, 64, "echo %i > /proc/sys/kernel/printk",vprintk);
		system(t_cmd);
	}

	for(k=0;k<MAX_PAGES;k++) {
		if(fd_bitmap&(1<<k))
			free_page(k);
	}
	if(ret < 0)
		exit(1);
	else
		exit(0);
}
