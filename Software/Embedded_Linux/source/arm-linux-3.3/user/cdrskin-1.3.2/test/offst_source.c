
/*
  cc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS -g -o test/offst_source test/offst_source.c -lburn
*/

#include "../libburn/libburn.h" 

/* Just everything from test/libburner.c */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>


static int create_original(struct burn_source **original, char *path, int flag)
{
	printf("create_original: path='%s'\n", path);
	*original = burn_file_source_new(path, NULL);
	if (*original == NULL)
		return 0;
	return 1;
}


static int set_up_offst_sources(struct burn_source *original,
				struct burn_source *offsetters[],
				int count, int flag)
{
	int i;
	off_t start = 3, size = 10, gap = 7;

	for (i = 0; i < count; i++) {
		offsetters[i] = burn_offst_source_new(original,
					i > 0 ? offsetters[i - 1] : NULL,
					start, size, 0);
		if (offsetters[i] == NULL)
			return 0;
		printf("set_up_offst_sources: idx=%d, start=%d\n",
			 i, (int) start);
		start += size + gap;
	}
	return 1;
}


static int consume_source(struct burn_source *src, int flag)
{
	int ret, count = 0;
	unsigned char buf[1];

	while (1) {
		ret = src->read_xt(src, buf, 1);
		if (ret < 0) {
			printf("\n");
			fprintf(stderr, "consume_source: count=%d, ret=%d\n",
					count, ret);
			return 0;
		}
		if (ret == 0)
	break;
		printf("%u ", buf[0]);
		count++;
	}
	printf(" count=%d\n", count);
	return 1;
}


static int consume_all_sources(struct burn_source *offsetters[],
                                int count, int flag)
{
	int i, ret;

	for (i = 0; i < count; i++) {
		printf("consume_source: idx=%d\n", i);
		ret = consume_source(offsetters[i], 0);
		if (ret <= 0)
			return ret;
	}
	return 1;
}


static int free_all_sources(struct burn_source *original,
                                struct burn_source *offsetters[],
                                int count, int flag)
{
	int i;

	for (i = 0; i < count; i++)
		burn_source_free(offsetters[i]);
	burn_source_free(original);
	return 1;
}


int main(int argc, char **argv)
{
	int ret;
	char *path = "./COPYRIGHT";
	struct burn_source *original = NULL, *offsetters[4];

	if (argc > 1)
		path = argv[1];

	if (burn_initialize() == 0)
		exit(1);

	ret = create_original(&original, path, 0);
	if (ret <= 0)
		exit(2);

	ret = set_up_offst_sources(original, offsetters, 4, 0);
	if (ret <= 0)
		exit(3);

	ret = consume_all_sources(offsetters, 4, 0);
	if (ret <= 0)
		exit(4);

	ret = free_all_sources(original, offsetters, 4, 0);
	if (ret <= 0)
		exit(5);

	burn_finish();
	exit(0);
}

