#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>

/* same as cpu_comm.h */
#define CPU_COMM_MAGIC 'h'
#define CPU_COMM_READ_MAX   _IOR(CPU_COMM_MAGIC, 1, unsigned int)
#define CPU_COMM_WRITE_MAX  _IOR(CPU_COMM_MAGIC, 2, unsigned int)

#define FA626_DEVICE  "/dev/cpucomm_FA626_chan0"
#define FA726_DEVICE  "/dev/cpucomm_FA726_chan0"

#define __version__ "1.3.1"

static struct cpucomm_header {
    char filename[30];
    int  size;
} header;

static void show_usage(void)
{
	printf("\nUsage: cpucomm_file -c <devchn> -f <filename> -rw\n"\
	    "   -c <devchn>  cpucomm channel name (eg. /dev/cpucomm_XXX_chanyyy)\n"\
		"   -f           File name to transfer.\n"\
		"   -r           Read data and create file.\n"\
		"   -w           Write data from file.\n\n");
}

int main(int argc, char **argv)
{
    char *device_name = NULL;
    unsigned int *memory = NULL, *tmp_memory = NULL;
    int cpucomm_fd = -1, ret = 0;
    FILE *ffile = NULL;
    struct stat st;
    int rt = 0, wt = 0, opt, status;
    unsigned int max_file_sz, left_sz, total_rcv_sz = 0;

    memset((void *)&header, 0, sizeof(header));

    while ((opt = getopt(argc, argv, "c:f:rw")) != -1) {
        switch (opt) {
          case 'c':
            device_name = optarg;
		    break;
		  case 'r':
			rt = 1;
			break;
		  case 'w':
			wt = 1;
			break;
          case 'f':
            ffile = fopen(optarg,"rb");
            if (ffile == NULL) {
                printf("cpucomm_file-" __version__ " open file fail! \n");
                return -1;
            }
            strcpy(header.filename, optarg);
            stat(header.filename, &st);
            header.size = st.st_size;
            /* allocate memory */
            memory = tmp_memory = malloc(header.size);
            if (!memory) {
                printf("cpucomm_file-" __version__ " allocate memory size %d fail! \n", header.size);
                fclose(ffile);
                return -1;
            }
            fread(memory, 1, st.st_size, ffile);
            fclose(ffile);
            break;
          default:
            show_usage();
            return 0;
        }
    }

    if (!device_name || (rt + wt) != 1) {
        show_usage();
        return 0;
    }
    if (!device_name || (wt && !header.size)) {
        show_usage();
        return 0;
    }

    if (rt == 1) {
        unsigned int rcv_sz;

        cpucomm_fd = open(device_name, O_RDONLY);
        if (cpucomm_fd < 0) {
            printf("cpucomm_file-" __version__ " open device node %s fail! \n", device_name);
            ret = -1;
            goto exit;
        }
        ioctl(cpucomm_fd, CPU_COMM_READ_MAX, &max_file_sz);
        status = read(cpucomm_fd, &header, sizeof(header));
        if (status <= 0) {
			printf("cpucomm_file-" __version__ " read header from cpucomm fail! \n");
			ret = -1;
			goto exit;
		}
		ffile = fopen(header.filename, "wb");
		if (ffile == 0) {
		    printf("cpucomm_file-" __version__ " creates file %s fail ! \n", header.filename);
			ret = -1;
			goto exit;
		}

		memory = tmp_memory = malloc(header.size);
        if (!memory) {
            printf("cpucomm_file-" __version__ " allocate memory size %d fail! \n", header.size);
            fclose(ffile);
            goto exit;
        }

        left_sz = header.size;

        do {
            rcv_sz = left_sz > max_file_sz ? max_file_sz : left_sz;
            status = read(cpucomm_fd, memory, rcv_sz);
            if (status <= 0) {
    			printf("cpucomm_file-" __version__ " read file body from cpucomm fail! \n");
    			ret = -1;
    			goto exit;
    		}
    		total_rcv_sz += status;
    		fwrite(memory, 1, rcv_sz, ffile);
            left_sz -= rcv_sz;
            memory = (unsigned int *)((unsigned int)memory + rcv_sz);
    	} while (left_sz);

        fclose(ffile);
        printf("cpucomm_file-" __version__ " creates file %s with size %d bytes. \n",
                                                            header.filename, total_rcv_sz);
        close(cpucomm_fd);
        /* prevent users recursively use cpucomm_file tool to send data */
        usleep(500000);
    }

    if (wt == 1) {
        unsigned int send_sz;

        cpucomm_fd = open(device_name, O_WRONLY);
        if (cpucomm_fd < 0) {
            printf("cpucomm_file-" __version__ " open device node %s fail! \n", device_name);
            ret = -1;
            goto exit;
        }
        ioctl(cpucomm_fd, CPU_COMM_WRITE_MAX, &max_file_sz);

        status = write(cpucomm_fd, &header, sizeof(header));
		if (status < 0) {
			printf("cpucomm_file-" __version__ " writes header to cpucomm fail! \n");
			ret = -1;
			goto exit;
		}

		left_sz = header.size;
		do {
		    send_sz = (left_sz > max_file_sz) ? max_file_sz : left_sz;
    		status = write(cpucomm_fd, memory, send_sz);
    		if (status < 0) {
    			printf("cpucomm_file-" __version__ " writes data to cpucomm fail! \n");
    			ret = -1;
    			goto exit;
    		}
    		left_sz -= send_sz;
    		memory = (unsigned int *)((unsigned int)memory + send_sz);
    	} while (left_sz);

		printf("cpucomm_file-" __version__ " writes file %s with size %d to cpucomm.\n",
		                                                    header.filename, header.size);
        close(cpucomm_fd);

        /* prevent users recursively use cpucomm_file tool to send data */
        usleep(500000);
    }
exit:
    memory = tmp_memory;
    if (memory)
        free(memory);
    return ret;
}

