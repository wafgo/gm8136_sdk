/*
 * Copyright (C) 2008 Nokia Corporation.
 * Copyright (C) 2008 University of Szeged, Hungary
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Authors: Adrian Hunter
 *          Artem Bityutskiy
 *          Zoltan Sogor
 */

#include "mkfs.ubifs.h"

#define PROGRAM_VERSION "1.1"

/* Size (prime number) of hash table for link counting */
#define HASH_TABLE_SIZE 10099

/* The node buffer must allow for worst case compression */
#define NODE_BUFFER_SIZE (UBIFS_DATA_NODE_SZ + \
			  UBIFS_BLOCK_SIZE * WORST_COMPR_FACTOR)

/* Default time granularity in nanoseconds */
#define DEFAULT_TIME_GRAN 1000000000

/**
 * struct idx_entry - index entry.
 * @next: next index entry (NULL at end of list)
 * @prev: previous index entry (NULL at beginning of list)
 * @key: key
 * @name: directory entry name used for sorting colliding keys by name
 * @lnum: LEB number
 * @offs: offset
 * @len: length
 *
 * The index is recorded as a linked list which is sorted and used to create
 * the bottom level of the on-flash index tree. The remaining levels of the
 * index tree are each built from the level below.
 */
struct idx_entry {
	struct idx_entry *next;
	struct idx_entry *prev;
	union ubifs_key key;
	char *name;
	int lnum;
	int offs;
	int len;
};

/**
 * struct inum_mapping - inode number mapping for link counting.
 * @next: next inum_mapping (NULL at end of list)
 * @prev: previous inum_mapping (NULL at beginning of list)
 * @dev: source device on which the source inode number resides
 * @inum: source inode number of the file
 * @use_inum: target inode number of the file
 * @use_nlink: number of links
 * @path_name: a path name of the file
 * @st: struct stat object containing inode attributes which have to be used
 *      when the inode is being created (actually only UID, GID, access
 *      mode, major and minor device numbers)
 *
 * If a file has more than one hard link, then the number of hard links that
 * exist in the source directory hierarchy must be counted to exclude the
 * possibility that the file is linked from outside the source directory
 * hierarchy.
 *
 * The inum_mappings are stored in a hash_table of linked lists.
 */
struct inum_mapping {
	struct inum_mapping *next;
	struct inum_mapping *prev;
	dev_t dev;
	ino_t inum;
	ino_t use_inum;
	unsigned int use_nlink;
	char *path_name;
	struct stat st;
};

/*
 * Because we copy functions from the kernel, we use a subset of the UBIFS
 * file-system description object struct ubifs_info.
 */
struct ubifs_info info_;
static struct ubifs_info *c = &info_;

/* Debug levels are: 0 (none), 1 (statistics), 2 (files) ,3 (more details) */
int debug_level;
int verbose;

static char *root;
static int root_len;
static struct stat root_st;
static char *output;
static int out_fd;
static int squash_owner;

/* The 'head' (position) which nodes are written */
static int head_lnum;
static int head_offs;
static int head_flags;

/* The index list */
static struct idx_entry *idx_list_first;
static struct idx_entry *idx_list_last;
static size_t idx_cnt;

/* Global buffers */
static void *leb_buf;
static void *node_buf;
static void *block_buf;

/* Hash table for inode link counting */
static struct inum_mapping **hash_table;

/* Inode creation sequence number */
static unsigned long long creat_sqnum;

static const char *optstring = "d:r:m:o:D:h?vVe:c:g:f:P:k:x:X:j:l:j:U";

static const struct option longopts[] = {
	{"root",          1, NULL, 'r'},
	{"min-io-size",   1, NULL, 'm'},
	{"leb-size",      1, NULL, 'e'},
	{"max-leb-cnt",   1, NULL, 'c'},
	{"output",        1, NULL, 'o'},
	{"devtable",      1, NULL, 'D'},
	{"help",          0, NULL, 'h'},
	{"verbose",       0, NULL, 'v'},
	{"version",       0, NULL, 'V'},
	{"debug-level",   1, NULL, 'g'},
	{"jrn-size",      1, NULL, 'j'},
	{"compr",         1, NULL, 'x'},
	{"favor-percent", 1, NULL, 'X'},
	{"fanout",        1, NULL, 'f'},
	{"keyhash",       1, NULL, 'k'},
	{"log-lebs",      1, NULL, 'l'},
	{"orph-lebs",     1, NULL, 'p'},
	{"squash-uids" ,  0, NULL, 'U'},
	{NULL, 0, NULL, 0}
};

static const char *helptext =
"Usage: mkfs.ubifs [OPTIONS]\n"
"Make a UBIFS file system image from an existing directory tree\n\n"
"Options:\n"
"-r, -d, --root=DIR       build file system from directory DIR\n"
"-m, --min-io-size=SIZE   minimum I/O unit size\n"
"-e, --leb-size=SIZE      logical erase block size\n"
"-c, --max-leb-cnt=COUNT  maximum logical erase block count\n"
"-o, --output=FILE        output to FILE\n"
"-j, --jrn-size=SIZE      journal size\n"
"-x, --compr=TYPE         compression type - \"lzo\", \"favor_lzo\", \"zlib\" or\n"
"                         \"none\" (default: \"lzo\")\n"
"-X, --favor-percent      may only be used with favor LZO compression and defines\n"
"                         how many percent better zlib should compress to make\n"
"                         mkfs.ubifs use zlib instead of LZO (default 20%)\n"
"-f, --fanout=NUM         fanout NUM (default: 8)\n"
"-k, --keyhash=TYPE       key hash type - \"r5\" or \"test\" (default: \"r5\")\n"
"-p, --orph-lebs=COUNT    count of erase blocks for orphans (default: 1)\n"
"-D, --devtable=FILE      use device table FILE\n"
"-U, --squash-uids        squash owners making all files owned by root\n"
"-l, --log-lebs=COUNT     count of erase blocks for the log (used only for\n"
"                         debugging)\n"
"-v, --verbose            verbose operation\n"
"-V, --version            display version information\n"
"-g, --debug=LEVEL        display debug information (0 - none, 1 - statistics,\n"
"                         2 - files, 3 - more details)\n"
"-h, --help               display this help text\n\n"
"Note, SIZE is specified in bytes, but it may also be specified in Kilobytes,\n"
"Megabytes, and Gigabytes if a KiB, MiB, or GiB suffix is used.\n\n"
"If you specify \"lzo\" or \"zlib\" compressors, mkfs.ubifs will use this compressor\n"
"for all data. The \"none\" disables any data compression. The \"favor_lzo\" is not\n"
"really a separate compressor. It is just a method of combining \"lzo\" and \"zlib\"\n"
"compressors. Namely, mkfs.ubifs tries to compress data with both \"lzo\" and \"zlib\"\n"
"compressors, then it compares which compressor is better. If \"zlib\" compresses 20\n"
"or more percent better than \"lzo\", mkfs.ubifs chooses \"lzo\", otherwise it chooses\n"
"\"zlib\". The \"--favor-percent\" may specify arbitrary threshold instead of the\n"
"default 20%.\n";

/**
 * make_path - make a path name from a directory and a name.
 * @dir: directory path name
 * @name: name
 */
static char *make_path(const char *dir, const char *name)
{
	char *s;

	s = malloc(strlen(dir) + strlen(name) + 2);
	if (!s)
		return NULL;
	strcpy(s, dir);
	if (dir[strlen(dir) - 1] != '/')
		strcat(s, "/");
	strcat(s, name);
	return s;
}

/**
 * same_dir - determine if two file descriptors refer to the same directory.
 * @fd1: file descriptor 1
 * @fd2: file descriptor 2
 */
static int same_dir(int fd1, int fd2)
{
	struct stat stat1, stat2;

	if (fstat(fd1, &stat1) == -1)
		return -1;
	if (fstat(fd2, &stat2) == -1)
		return -1;
	return stat1.st_dev == stat2.st_dev && stat1.st_ino == stat2.st_ino;
}

/**
 * do_openat - open a file in a directory.
 * @fd: file descriptor of open directory
 * @path: path relative to directory
 * @flags: open flags
 *
 * This function is provided because the library function openat is sometimes
 * not available.
 */
static int do_openat(int fd, const char *path, int flags)
{
	int ret;
	char *cwd;

	cwd = getcwd(NULL, 0);
	if (!cwd)
		return -1;
	ret = fchdir(fd);
	if (ret != -1)
		ret = open(path, flags);
	chdir(cwd);
	free(cwd);
	return ret;
}

/**
 * in_path - determine if a file is beneath a directory.
 * @dir_name: directory path name
 * @file_name: file path name
 */
static int in_path(const char *dir_name, const char *file_name)
{
	char *fn = strdup(file_name);
	char *dn;
	int fd1, fd2, fd3, ret = -1, top_fd;

	if (!fn)
		return -1;
	top_fd = open("/", O_RDONLY);
	if (top_fd != -1) {
		dn = dirname(fn);
		fd1 = open(dir_name, O_RDONLY);
		if (fd1 != -1) {
			fd2 = open(dn, O_RDONLY);
			if (fd2 != -1) {
				while (1) {
					int same;

					same = same_dir(fd1, fd2);
					if (same) {
						ret = same;
						break;
					}
					if (same_dir(fd2, top_fd)) {
						ret = 0;
						break;
					}
					fd3 = do_openat(fd2, "..", O_RDONLY);
					if (fd3 == -1)
						break;
					close(fd2);
					fd2 = fd3;
				}
				close(fd2);
			}
			close(fd1);
		}
		close(top_fd);
	}
	free(fn);
	return ret;
}

/**
 * calc_min_log_lebs - calculate the minimum number of log LEBs needed.
 * @max_bud_bytes: journal size (buds only)
 */
static int calc_min_log_lebs(unsigned long long max_bud_bytes)
{
	int buds, log_lebs;
	unsigned long long log_size;

	buds = (max_bud_bytes + c->leb_size - 1) / c->leb_size;
	log_size = ALIGN(UBIFS_REF_NODE_SZ, c->min_io_size);
	log_size *= buds;
	log_size += ALIGN(UBIFS_CS_NODE_SZ +
			  UBIFS_REF_NODE_SZ * (c->jhead_cnt + 2),
			  c->min_io_size);
	log_lebs = (log_size + c->leb_size - 1) / c->leb_size;
	log_lebs += 1;
	return log_lebs;
}

static inline int is_power_of_2(unsigned long long n)
{
                return (n != 0 && ((n & (n - 1)) == 0));
}

static int validate_options(void)
{
	int tmp;

	if (!root)
		return err_msg("root directory was not specified");
	if (!output)
		return err_msg("no output file specified");
	if (in_path(root, output))
		return err_msg("output file cannot be in the UBIFS root "
			       "directory");
	if (!is_power_of_2(c->min_io_size))
		return err_msg("min. I/O unit size should be power of 2");
	if (c->leb_size < c->min_io_size)
		return err_msg("min. I/O unit cannot be larger than LEB size");
	if (c->leb_size < UBIFS_MIN_LEB_SZ)
		return err_msg("too smal LEB size %d, minimum is %d",
			       c->min_io_size, UBIFS_MIN_LEB_SZ);
	if (c->leb_size % c->min_io_size)
		return err_msg("LEB should be multiple of min. I/O units");
	if (c->leb_size % 8)
		return err_msg("LEB size has to be multiple of 8");
	if (c->leb_size > 1024*1024)
		return err_msg("too large LEB size %d", c->leb_size);
	if (c->max_leb_cnt < UBIFS_MIN_LEB_CNT)
		return err_msg("too low max. count of LEBs, minimum is %d",
			       UBIFS_MIN_LEB_CNT);
	if (c->fanout < UBIFS_MIN_FANOUT)
		return err_msg("too low fanout, minimum is %d",
			       UBIFS_MIN_FANOUT);
	tmp = c->leb_size - UBIFS_IDX_NODE_SZ;
	tmp /= UBIFS_BRANCH_SZ + UBIFS_MAX_KEY_LEN;
	if (c->fanout > tmp)
		return err_msg("too high fanout, maximum is %d", tmp);
	if (c->log_lebs < UBIFS_MIN_LOG_LEBS)
		return err_msg("too few log LEBs, minimum is %d",
			       UBIFS_MIN_LOG_LEBS);
	if (c->log_lebs >= c->max_leb_cnt - UBIFS_MIN_LEB_CNT)
		return err_msg("too many log LEBs, maximum is %d",
			       c->max_leb_cnt - UBIFS_MIN_LEB_CNT);
	if (c->orph_lebs < UBIFS_MIN_ORPH_LEBS)
		return err_msg("too few orphan LEBs, minimum is %d",
			       UBIFS_MIN_ORPH_LEBS);
	if (c->orph_lebs >= c->max_leb_cnt - UBIFS_MIN_LEB_CNT)
		return err_msg("too many orphan LEBs, maximum is %d",
			       c->max_leb_cnt - UBIFS_MIN_LEB_CNT);
	tmp = UBIFS_SB_LEBS + UBIFS_MST_LEBS + c->log_lebs + c->lpt_lebs;
	tmp += c->orph_lebs + 4;
	if (tmp > c->max_leb_cnt)
		return err_msg("too low max. count of LEBs, expected at "
			       "least %d", tmp);
	tmp = calc_min_log_lebs(c->max_bud_bytes);
	if (c->log_lebs < calc_min_log_lebs(c->max_bud_bytes))
		return err_msg("too few log LEBs, expected at least %d",
			       tmp);
	return 0;
}

/**
 * get_multiplier - convert size specifier to an integer multiplier.
 * @str: the size specifier string
 *
 * This function parses the @str size specifier, which may be one of
 * 'KiB', 'MiB', or 'GiB' into an integer multiplier. Returns positive
 * size multiplier in case of success and %-1 in case of failure.
 */
static int get_multiplier(const char *str)
{
	if (!str)
		return 1;

	/* Remove spaces before the specifier */
	while (*str == ' ' || *str == '\t')
		str += 1;

	if (!strcmp(str, "KiB"))
		return 1024;
	if (!strcmp(str, "MiB"))
		return 1024 * 1024;
	if (!strcmp(str, "GiB"))
		return 1024 * 1024 * 1024;

	return -1;
}

/**
 * get_bytes - convert a string containing amount of bytes into an
 *             integer.
 * @str: string to convert
 *
 * This function parses @str which may have one of 'KiB', 'MiB', or 'GiB' size
 * specifiers. Returns positive amount of bytes in case of success and %-1 in
 * case of failure.
 */
static long long get_bytes(const char *str)
{
	char *endp;
	long long bytes = strtoull(str, &endp, 0);

	if (endp == str || bytes < 0)
		return err_msg("incorrect amount of bytes: \"%s\"", str);

	if (*endp != '\0') {
		int mult = get_multiplier(endp);

		if (mult == -1)
			return err_msg("bad size specifier: \"%s\" - "
				       "should be 'KiB', 'MiB' or 'GiB'", endp);
		bytes *= mult;
	}

	return bytes;
}

static int get_options(int argc, char**argv)
{
	int opt, i;
	const char *tbl_file = NULL;
	struct stat st;
	char *endp;

	c->fanout = 8;
	c->orph_lebs = 1;
	c->key_hash = key_r5_hash;
	c->key_len = UBIFS_SK_LEN;
	c->default_compr = UBIFS_COMPR_LZO;
	c->favor_percent = 20;
	c->lsave_cnt = 256;
	c->leb_size = -1;
	c->min_io_size = -1;
	c->max_leb_cnt = -1;
	c->max_bud_bytes = -1;
	c->log_lebs = -1;

	while (1) {
		opt = getopt_long(argc, argv, optstring, longopts, &i);
		if (opt == -1)
			break;
		switch (opt) {
		case 'r':
		case 'd':
			root_len = strlen(optarg);
			root = malloc(root_len + 2);
			if (!root)
				return err_msg("cannot allocate memory");

			/*
			 * The further code expects '/' at the end of the root
			 * UBIFS directory on the host.
			 */
			memcpy(root, optarg, root_len);
			if (root[root_len - 1] != '/')
				root[root_len++] = '/';
			root[root_len] = 0;

			/* Make sure the root directory exists */
			if (stat(root, &st))
				return sys_err_msg("bad root directory '%s'",
						   root);
			break;
		case 'm':
			c->min_io_size = get_bytes(optarg);
			if (c->min_io_size <= 0)
				return err_msg("bad min. I/O size");
			break;
		case 'e':
			c->leb_size = get_bytes(optarg);
			if (c->leb_size <= 0)
				return err_msg("bad LEB size");
			break;
		case 'c':
			c->max_leb_cnt = get_bytes(optarg);
			if (c->max_leb_cnt <= 0)
				return err_msg("bad maximum LEB count");
			break;
		case 'o':
			output = strdup(optarg);
			break;
		case 'D':
			tbl_file = optarg;
			if (stat(tbl_file, &st) < 0)
				return sys_err_msg("bad device table file '%s'",
						   tbl_file);
			break;
		case 'h':
		case '?':
			printf(helptext);
			exit(0);
		case 'v':
			verbose = 1;
			break;
		case 'V':
			printf("Version " PROGRAM_VERSION "\n");
			exit(0);
		case 'g':
			debug_level = strtol(optarg, &endp, 0);
			if (*endp != '\0' || endp == optarg ||
			    debug_level < 0 || debug_level > 3)
				return err_msg("bad debugging level '%s'",
					       optarg);
			break;
		case 'f':
			c->fanout = strtol(optarg, &endp, 0);
			if (*endp != '\0' || endp == optarg || c->fanout <= 0)
				return err_msg("bad fanout %s", optarg);
			break;
		case 'l':
			c->log_lebs = strtol(optarg, &endp, 0);
			if (*endp != '\0' || endp == optarg || c->log_lebs <= 0)
				return err_msg("bad count of log LEBs '%s'",
					       optarg);
			break;
		case 'p':
			c->orph_lebs = strtol(optarg, &endp, 0);
			if (*endp != '\0' || endp == optarg ||
			    c->orph_lebs <= 0)
				return err_msg("bad orphan LEB count '%s'",
					       optarg);
			break;
		case 'k':
			if (strcmp(optarg, "r5") == 0) {
				c->key_hash = key_r5_hash;
				c->key_hash_type = UBIFS_KEY_HASH_R5;
			} else if (strcmp(optarg, "test") == 0) {
				c->key_hash = key_test_hash;
				c->key_hash_type = UBIFS_KEY_HASH_TEST;
			} else
				return err_msg("bad key hash");
			break;
		case 'x':
			if (strcmp(optarg, "favor_lzo") == 0)
				c->favor_lzo = 1;
			else if (strcmp(optarg, "zlib") == 0)
				c->default_compr = UBIFS_COMPR_ZLIB;
			else if (strcmp(optarg, "none") == 0)
				c->default_compr = UBIFS_COMPR_NONE;
			else if (strcmp(optarg, "lzo") != 0)
				return err_msg("bad compressor name");
			break;
		case 'X':
			c->favor_percent = strtol(optarg, &endp, 0);
			if (*endp != '\0' || endp == optarg ||
			    c->favor_percent <= 0 || c->favor_percent >= 100)
				return err_msg("bad favor LZO percent '%s'",
					       optarg);
			break;
		case 'j':
			c->max_bud_bytes = get_bytes(optarg);
			if (c->max_bud_bytes <= 0)
				return err_msg("bad maximum amount of buds");
			break;
		case 'U':
			squash_owner = 1;
			break;
		}
	}

	if (c->min_io_size == -1)
		return err_msg("min. I/O unit was not specified "
			       "(use -h for help)");

	if (c->leb_size == -1)
		return err_msg("LEB size was not specified (use -h for help)");

	if (c->max_leb_cnt == -1)
		return err_msg("Maximum count of LEBs was not specified "
			       "(use -h for help)");

	if (c->max_bud_bytes == -1) {
		int lebs;

		lebs = c->max_leb_cnt - UBIFS_SB_LEBS - UBIFS_MST_LEBS;
		lebs -= c->orph_lebs;
		if (c->log_lebs != -1)
			lebs -= c->log_lebs;
		else
			lebs -= UBIFS_MIN_LOG_LEBS;
		/*
		 * We do not know lprops geometry so far, so assume minimum
		 * count of lprops LEBs.
		 */
		lebs -= UBIFS_MIN_LPT_LEBS;
		/* Make the journal about 12.5% of main area lebs */
		c->max_bud_bytes = (lebs / 8) * (long long)c->leb_size;
		/* Make the max journal size 8MiB */
		if (c->max_bud_bytes > 8 * 1024 * 1024)
			c->max_bud_bytes = 8 * 1024 * 1024;
		if (c->max_bud_bytes < 4 * c->leb_size)
			c->max_bud_bytes = 4 * c->leb_size;
	}

	if (c->log_lebs == -1) {
		c->log_lebs = calc_min_log_lebs(c->max_bud_bytes);
		c->log_lebs += 2;
	}

	if (verbose) {
		printf("mkfs.ubifs\n");
		printf("\troot:         %s\n", root);
		printf("\tmin_io_size:  %d\n", c->min_io_size);
		printf("\tleb_size:     %d\n", c->leb_size);
		printf("\tmax_leb_cnt:  %d\n", c->max_leb_cnt);
		printf("\toutput:       %s\n", output);
		printf("\tjrn_size:     %llu\n", c->max_bud_bytes);
		switch (c->default_compr) {
		case UBIFS_COMPR_LZO:
			printf("\tcompr:        lzo\n");
			break;
		case UBIFS_COMPR_ZLIB:
			printf("\tcompr:        zlib\n");
			break;
		case UBIFS_COMPR_NONE:
			printf("\tcompr:        none\n");
			break;
		}
		printf("\tkeyhash:      %s\n", (c->key_hash == key_r5_hash) ?
						"r5" : "test");
		printf("\tfanout:       %d\n", c->fanout);
		printf("\torph_lebs:    %d\n", c->orph_lebs);
	}

	if (c->min_io_size < 8)
		c->min_io_size = 8;

	if (validate_options())
		return -1;

	if (tbl_file && parse_devtable(root, tbl_file))
		return err_msg("cannot parse device table file '%s'", tbl_file);

	return 0;
}

/**
 * prepare_node - fill in the common header.
 * @node: node
 * @len: node length
 */
static void prepare_node(void *node, int len)
{
	uint32_t crc;
	struct ubifs_ch *ch = node;

	ch->magic = cpu_to_le32(UBIFS_NODE_MAGIC);
	ch->len = cpu_to_le32(len);
	ch->group_type = UBIFS_NO_NODE_GROUP;
	ch->sqnum = cpu_to_le64(++c->max_sqnum);
	ch->padding[0] = ch->padding[1] = 0;
	crc = ubifs_crc32(UBIFS_CRC32_INIT, node + 8, len - 8);
	ch->crc = cpu_to_le32(crc);
}

/**
 * write_leb - copy the image of a LEB to the output file.
 * @lnum: LEB number
 * @len: length of data in the buffer
 * @buf: buffer (must be at least c->leb_size bytes)
 */
int write_leb(int lnum, int len, void *buf)
{
	off64_t pos = (off64_t)lnum * c->leb_size;

	dbg_msg(3, "LEB %d len %d", lnum, len);
	if (lseek64(out_fd, pos, SEEK_SET) != pos)
		return sys_err_msg("lseek64 failed seeking %lld", pos);

	memset(buf + len, 0xff, c->leb_size - len);

	if (write(out_fd, buf, c->leb_size) != c->leb_size)
		return sys_err_msg("write failed writing %d bytes at pos %lld",
				   c->leb_size, pos);

	return 0;
}

/**
 * write_empty_leb - copy the image of an empty LEB to the output file.
 * @lnum: LEB number
 */
static int write_empty_leb(int lnum)
{
	return write_leb(lnum, 0, leb_buf);
}

/**
 * do_pad - pad a buffer to the minimum I/O size.
 * @buf: buffer
 * @len: buffer length
 */
static int do_pad(void *buf, int len)
{
	int pad_len, alen = ALIGN(len, 8), wlen = ALIGN(alen, c->min_io_size);
	uint32_t crc;

	memset(buf + len, 0xff, alen - len);
	pad_len = wlen - alen;
	dbg_msg(3, "len %d pad_len %d", len, pad_len);
	buf += alen;
	if (pad_len >= UBIFS_PAD_NODE_SZ) {
		struct ubifs_ch *ch = buf;
		struct ubifs_pad_node *pad_node = buf;

		ch->magic      = cpu_to_le32(UBIFS_NODE_MAGIC);
		ch->node_type  = UBIFS_PAD_NODE;
		ch->group_type = UBIFS_NO_NODE_GROUP;
		ch->padding[0] = ch->padding[1] = 0;
		ch->sqnum      = cpu_to_le64(0);
		ch->len        = cpu_to_le32(UBIFS_PAD_NODE_SZ);

		pad_len -= UBIFS_PAD_NODE_SZ;
		pad_node->pad_len = cpu_to_le32(pad_len);

		crc = ubifs_crc32(UBIFS_CRC32_INIT, buf + 8,
				  UBIFS_PAD_NODE_SZ - 8);
		ch->crc = cpu_to_le32(crc);

		memset(buf + UBIFS_PAD_NODE_SZ, 0, pad_len);
	} else if (pad_len > 0)
		memset(buf, UBIFS_PADDING_BYTE, pad_len);

	return wlen;
}

/**
 * write_node - write a node to a LEB.
 * @node: node
 * @len: node length
 * @lnum: LEB number
 */
static int write_node(void *node, int len, int lnum)
{
	prepare_node(node, len);

	memcpy(leb_buf, node, len);

	len = do_pad(leb_buf, len);

	return write_leb(lnum, len, leb_buf);
}

/**
 * calc_dark - calculate LEB dark space size.
 * @c: the UBIFS file-system description object
 * @spc: amount of free and dirty space in the LEB
 *
 * This function calculates amount of dark space in an LEB which has @spc bytes
 * of free and dirty space. Returns the calculations result.
 *
 * Dark space is the space which is not always usable - it depends on which
 * nodes are written in which order. E.g., if an LEB has only 512 free bytes,
 * it is dark space, because it cannot fit a large data node. So UBIFS cannot
 * count on this LEB and treat these 512 bytes as usable because it is not true
 * if, for example, only big chunks of uncompressible data will be written to
 * the FS.
 */
static int calc_dark(struct ubifs_info *c, int spc)
{
	if (spc < c->dark_wm)
		return spc;

	/*
	 * If we have slightly more space then the dark space watermark, we can
	 * anyway safely assume it we'll be able to write a node of the
	 * smallest size there.
	 */
	if (spc - c->dark_wm < MIN_WRITE_SZ)
		return spc - MIN_WRITE_SZ;

	return c->dark_wm;
}

/**
 * set_lprops - set the LEB property values for a LEB.
 * @lnum: LEB number
 * @offs: end offset of data in the LEB
 * @flags: LEB property flags
 */
static void set_lprops(int lnum, int offs, int flags)
{
	int i = lnum - c->main_first, free, dirty;
	int a = max_t(int, c->min_io_size, 8);

	free = c->leb_size - ALIGN(offs, a);
	dirty = c->leb_size - free - ALIGN(offs, 8);
	dbg_msg(3, "LEB %d free %d dirty %d flags %d", lnum, free, dirty,
		flags);
	c->lpt[i].free = free;
	c->lpt[i].dirty = dirty;
	c->lpt[i].flags = flags;
	c->lst.total_free += free;
	c->lst.total_dirty += dirty;
	if (flags & LPROPS_INDEX)
		c->lst.idx_lebs += 1;
	else {
		int spc;

		spc = free + dirty;
		if (spc < c->dead_wm)
			c->lst.total_dead += spc;
		else
			c->lst.total_dark += calc_dark(c, spc);
		c->lst.total_used += c->leb_size - spc;
	}
}

/**
 * add_to_index - add a node key and position to the index.
 * @key: node key
 * @lnum: node LEB number
 * @offs: node offset
 * @len: node length
 */
static int add_to_index(union ubifs_key *key, char *name, int lnum, int offs,
			int len)
{
	struct idx_entry *e;

	dbg_msg(3, "LEB %d offs %d len %d", lnum, offs, len);
	e = malloc(sizeof(struct idx_entry));
	if (!e)
		return err_msg("out of memory");
	e->next = NULL;
	e->prev = idx_list_last;
	e->key = *key;
	e->name = name;
	e->lnum = lnum;
	e->offs = offs;
	e->len = len;
	if (!idx_list_first)
		idx_list_first = e;
	if (idx_list_last)
		idx_list_last->next = e;
	idx_list_last = e;
	idx_cnt += 1;
	return 0;
}

/**
 * flush_nodes - write the current head and move the head to the next LEB.
 */
static int flush_nodes(void)
{
	int len, err;

	if (!head_offs)
		return 0;
	len = do_pad(leb_buf, head_offs);
	err = write_leb(head_lnum, len, leb_buf);
	if (err)
		return err;
	set_lprops(head_lnum, head_offs, head_flags);
	head_lnum += 1;
	head_offs = 0;
	return 0;
}

/**
 * reserve_space - reserve space for a node on the head.
 * @len: node length
 * @lnum: LEB number is returned here
 * @offs: offset is returned here
 */
static int reserve_space(int len, int *lnum, int *offs)
{
	int err;

	if (len > c->leb_size - head_offs) {
		err = flush_nodes();
		if (err)
			return err;
	}
	*lnum = head_lnum;
	*offs = head_offs;
	head_offs += ALIGN(len, 8);
	return 0;
}

/**
 * add_node - write a node to the head.
 * @key: node key
 * @node: node
 * @len: node length
 */
static int add_node(union ubifs_key *key, char *name, void *node, int len)
{
	int err, lnum, offs;

	prepare_node(node, len);

	err = reserve_space(len, &lnum, &offs);
	if (err)
		return err;

	memcpy(leb_buf + offs, node, len);
	memset(leb_buf + offs + len, 0xff, ALIGN(len, 8) - len);

	add_to_index(key, name, lnum, offs, len);

	return 0;
}

/**
 * add_inode_with_data - write an inode.
 * @st: stat information of source inode
 * @inum: target inode number
 * @data: inode data (for special inodes e.g. symlink path etc)
 * @data_len: inode data length
 * @flags: source inode flags
 */
static int add_inode_with_data(struct stat *st, ino_t inum, void *data,
			       unsigned int data_len, int flags)
{
	struct ubifs_ino_node *ino = node_buf;
	union ubifs_key key;
	int len, use_flags = 0;

	if (c->default_compr != UBIFS_COMPR_NONE)
		use_flags |= UBIFS_COMPR_FL;
	if (flags & FS_COMPR_FL)
		use_flags |= UBIFS_COMPR_FL;
	if (flags & FS_SYNC_FL)
		use_flags |= UBIFS_SYNC_FL;
	if (flags & FS_IMMUTABLE_FL)
		use_flags |= UBIFS_IMMUTABLE_FL;
	if (flags & FS_APPEND_FL)
		use_flags |= UBIFS_APPEND_FL;
	if (flags & FS_DIRSYNC_FL && S_ISDIR(st->st_mode))
		use_flags |= UBIFS_DIRSYNC_FL;

	memset(ino, 0, UBIFS_INO_NODE_SZ);

	ino_key_init(c, &key, inum);
	ino->ch.node_type = UBIFS_INO_NODE;
	key_write(c, &key, &ino->key);
	ino->creat_sqnum = cpu_to_le64(creat_sqnum);
	ino->size       = cpu_to_le64(st->st_size);
	ino->nlink      = cpu_to_le32(st->st_nlink);
	/*
	 * The time fields are updated assuming the default time granularity
	 * of 1 second. To support finer granularities, utime() would be needed.
	 */
	ino->atime_sec  = cpu_to_le64(st->st_atime);
	ino->ctime_sec  = cpu_to_le64(st->st_ctime);
	ino->mtime_sec  = cpu_to_le64(st->st_mtime);
	ino->atime_nsec = 0;
	ino->ctime_nsec = 0;
	ino->mtime_nsec = 0;
	ino->uid        = cpu_to_le32(st->st_uid);
	ino->gid        = cpu_to_le32(st->st_gid);
	ino->uid        = cpu_to_le32(st->st_uid);
	ino->gid        = cpu_to_le32(st->st_gid);
	ino->mode       = cpu_to_le32(st->st_mode);
	ino->flags      = cpu_to_le32(use_flags);
	ino->data_len   = cpu_to_le32(data_len);
	ino->compr_type = cpu_to_le16(c->default_compr);
	if (data_len)
		memcpy(&ino->data, data, data_len);

	len = UBIFS_INO_NODE_SZ + data_len;

	return add_node(&key, NULL, ino, len);
}

/**
 * add_inode - write an inode.
 * @st: stat information of source inode
 * @inum: target inode number
 * @flags: source inode flags
 */
static int add_inode(struct stat *st, ino_t inum, int flags)
{
	return add_inode_with_data(st, inum, NULL, 0, flags);
}

/**
 * add_dir_inode - write an inode for a directory.
 * @dir: source directory
 * @inum: target inode number
 * @size: target directory size
 * @nlink: target directory link count
 * @st: struct stat object describing attributes (except size and nlink) of the
 *      target inode to create
 *
 * Note, this function may be called with %NULL @dir, when the directory which
 * is being created does not exist at the host file system, but is defined by
 * the device table.
 */
static int add_dir_inode(DIR *dir, ino_t inum, loff_t size, unsigned int nlink,
			 struct stat *st)
{
	int fd, flags = 0;

	st->st_size = size;
	st->st_nlink = nlink;

	if (dir) {
		fd = dirfd(dir);
		if (fd == -1)
			return sys_err_msg("dirfd failed");
		if (ioctl(fd, FS_IOC_GETFLAGS, &flags) == -1)
			flags = 0;
	}

	return add_inode(st, inum, flags);
}

/**
 * add_dev_inode - write an inode for a character or block device.
 * @st: stat information of source inode
 * @inum: target inode number
 * @flags: source inode flags
 */
static int add_dev_inode(struct stat *st, ino_t inum, int flags)
{
	union ubifs_dev_desc dev;

	dev.huge = cpu_to_le64(makedev(major(st->st_rdev), minor(st->st_rdev)));
	return add_inode_with_data(st, inum, &dev, 8, flags);
}

/**
 * add_symlink_inode - write an inode for a symbolic link.
 * @path_name: path name of symbolic link inode itself (not the link target)
 * @st: stat information of source inode
 * @inum: target inode number
 * @flags: source inode flags
 */
static int add_symlink_inode(const char *path_name, struct stat *st, ino_t inum,
			     int flags)
{
	char buf[UBIFS_MAX_INO_DATA + 2];
	ssize_t len;

	/* Take the symlink as is */
	len = readlink(path_name, buf, UBIFS_MAX_INO_DATA + 1);
	if (len <= 0)
		return sys_err_msg("readlink failed for %s", path_name);
	if (len > UBIFS_MAX_INO_DATA)
		return err_msg("symlink too long for %s", path_name);

	return add_inode_with_data(st, inum, buf, len, flags);
}

/**
 * add_dent_node - write a directory entry node.
 * @dir_inum: target inode number of directory
 * @name: directory entry name
 * @inum: target inode number of the directory entry
 * @type: type of the target inode
 */
static int add_dent_node(ino_t dir_inum, const char *name, ino_t inum,
			 unsigned char type)
{
	struct ubifs_dent_node *dent = node_buf;
	union ubifs_key key;
	struct qstr dname;
	char *kname;
	int len;

	dbg_msg(3, "%s ino %lu type %u dir ino %lu", name, inum,
		(unsigned)type, dir_inum);
	memset(dent, 0, UBIFS_DENT_NODE_SZ);

	dname.name = (void *)name;
	dname.len = strlen(name);

	dent->ch.node_type = UBIFS_DENT_NODE;

	dent_key_init(c, &key, dir_inum, &dname);
	key_write(c, &key, dent->key);
	dent->inum = cpu_to_le64(inum);
	dent->padding1 = 0;
	dent->type = type;
	dent->nlen = cpu_to_le16(dname.len);
	memcpy(dent->name, dname.name, dname.len);
	dent->name[dname.len] = '\0';

	len = UBIFS_DENT_NODE_SZ + dname.len + 1;

	kname = strdup(name);
	if (!kname)
		return err_msg("cannot allocate memory");

	return add_node(&key, kname, dent, len);
}

/**
 * lookup_inum_mapping - add an inode mapping for link counting.
 * @dev: source device on which source inode number resides
 * @inum: source inode number
 */
static struct inum_mapping *lookup_inum_mapping(dev_t dev, ino_t inum)
{
	struct inum_mapping *im;
	unsigned int k;

	k = inum % HASH_TABLE_SIZE;
	im = hash_table[k];
	while (im) {
		if (im->dev == dev && im->inum == inum)
			return im;
		im = im->next;
	}
	im = malloc(sizeof(struct inum_mapping));
	if (!im)
		return NULL;
	im->next = hash_table[k];
	im->prev = NULL;
	im->dev = dev;
	im->inum = inum;
	im->use_inum = 0;
	im->use_nlink = 0;
	if (hash_table[k])
		hash_table[k]->prev = im;
	hash_table[k] = im;
	return im;
}

/**
 * all_zero - does a buffer contain only zero bytes.
 * @buf: buffer
 * @len: buffer length
 */
static int all_zero(void *buf, int len)
{
	unsigned char *p = buf;

	while (len--)
		if (*p++ != 0)
			return 0;
	return 1;
}

/**
 * add_file - write the data of a file and its inode to the output file.
 * @path_name: source path name
 * @st: source inode stat information
 * @inum: target inode number
 * @flags: source inode flags
 */
static int add_file(const char *path_name, struct stat *st, ino_t inum,
		    int flags)
{
	struct ubifs_data_node *dn = node_buf;
	void *buf = block_buf;
	loff_t file_size = 0;
	ssize_t ret, bytes_read;
	union ubifs_key key;
	int fd, dn_len, err, compr_type, use_compr;
	unsigned int block_no = 0;
	size_t out_len;

	fd = open(path_name, O_RDONLY | O_LARGEFILE);
	if (fd == -1)
		return sys_err_msg("failed to open file '%s'", path_name);
	do {
		/* Read next block */
		bytes_read = 0;
		do {
			ret = read(fd, buf + bytes_read,
				   UBIFS_BLOCK_SIZE - bytes_read);
			if (ret == -1) {
				sys_err_msg("failed to read file '%s'",
					    path_name);
				close(fd);
				return 1;
			}
			bytes_read += ret;
		} while (ret != 0 && bytes_read != UBIFS_BLOCK_SIZE);
		if (bytes_read == 0)
			break;
		file_size += bytes_read;
		/* Skip holes */
		if (all_zero(buf, bytes_read)) {
			block_no += 1;
			continue;
		}
		/* Make data node */
		memset(dn, 0, UBIFS_DATA_NODE_SZ);
		data_key_init(c, &key, inum, block_no++);
		dn->ch.node_type = UBIFS_DATA_NODE;
		key_write(c, &key, &dn->key);
		dn->size = cpu_to_le32(bytes_read);
		out_len = NODE_BUFFER_SIZE - UBIFS_DATA_NODE_SZ;
		if (c->default_compr == UBIFS_COMPR_NONE &&
		    (flags & FS_COMPR_FL))
			use_compr = UBIFS_COMPR_LZO;
		else
			use_compr = c->default_compr;
		compr_type = compress_data(buf, bytes_read, &dn->data,
					   &out_len, use_compr);
		dn->compr_type = cpu_to_le16(compr_type);
		dn_len = UBIFS_DATA_NODE_SZ + out_len;
		/* Add data node to file system */
		err = add_node(&key, NULL, dn, dn_len);
		if (err) {
			close(fd);
			return err;
		}
	} while (ret != 0);
	if (close(fd) == -1)
		return sys_err_msg("failed to close file '%s'", path_name);
	if (file_size != st->st_size)
		return err_msg("file size changed during writing file '%s'",
			       path_name);
	return add_inode(st, inum, flags);
}

/**
 * add_non_dir - write a non-directory to the output file.
 * @path_name: source path name
 * @inum: target inode number is passed and returned here (due to link counting)
 * @nlink: number of links if known otherwise zero
 * @type: UBIFS inode type is returned here
 * @st: struct stat object containing inode attributes which should be use when
 *      creating the UBIFS inode
 */
static int add_non_dir(const char *path_name, ino_t *inum, unsigned int nlink,
		       unsigned char *type, struct stat *st)
{
	int fd, flags = 0;

	dbg_msg(2, "%s", path_name);

	if (S_ISREG(st->st_mode)) {
		fd = open(path_name, O_RDONLY);
		if (fd == -1)
			return sys_err_msg("failed to open file '%s'",
					   path_name);
		if (ioctl(fd, FS_IOC_GETFLAGS, &flags) == -1)
			flags = 0;
		if (close(fd) == -1)
			return sys_err_msg("failed to close file '%s'",
					   path_name);
		*type = UBIFS_ITYPE_REG;
	} else if (S_ISCHR(st->st_mode))
		*type = UBIFS_ITYPE_CHR;
	else if (S_ISBLK(st->st_mode))
		*type = UBIFS_ITYPE_BLK;
	else if (S_ISLNK(st->st_mode))
		*type = UBIFS_ITYPE_LNK;
	else if (S_ISSOCK(st->st_mode))
		*type = UBIFS_ITYPE_SOCK;
	else if (S_ISFIFO(st->st_mode))
		*type = UBIFS_ITYPE_FIFO;
	else
		return err_msg("file '%s' has unknown inode type", path_name);

	if (nlink)
		st->st_nlink = nlink;
	else if (st->st_nlink > 1) {
		/*
		 * If the number of links is greater than 1, then add this file
		 * later when we know the number of links that we actually have.
		 * For now, we just put the inode mapping in the hash table.
		 */
		struct inum_mapping *im;

		im = lookup_inum_mapping(st->st_dev, st->st_ino);
		if (!im)
			return err_msg("out of memory");
		if (im->use_nlink == 0) {
			/* New entry */
			im->use_inum = *inum;
			im->use_nlink = 1;
			im->path_name = malloc(strlen(path_name) + 1);
			if (!im->path_name)
				return err_msg("out of memory");
			strcpy(im->path_name, path_name);
		} else {
			/* Existing entry */
			*inum = im->use_inum;
			im->use_nlink += 1;
			/* Return unused inode number */
			c->highest_inum -= 1;
		}

		memcpy(&im->st, st, sizeof(struct stat));
		return 0;
	} else
		st->st_nlink = 1;

	creat_sqnum = ++c->max_sqnum;

	if (S_ISREG(st->st_mode))
		return add_file(path_name, st, *inum, flags);
	if (S_ISCHR(st->st_mode))
		return add_dev_inode(st, *inum, flags);
	if (S_ISBLK(st->st_mode))
		return add_dev_inode(st, *inum, flags);
	if (S_ISLNK(st->st_mode))
		return add_symlink_inode(path_name, st, *inum, flags);
	if (S_ISSOCK(st->st_mode))
		return add_inode(st, *inum, flags);
	if (S_ISFIFO(st->st_mode))
		return add_inode(st, *inum, flags);

	return err_msg("file '%s' has unknown inode type", path_name);
}

/**
 * add_directory - write a directory tree to the output file.
 * @dir_name: directory path name
 * @dir_inum: UBIFS inode number of directory
 * @st: directory inode statistics
 * @non_existing: non-zero if this function is called for a directory which
 *                does not exist on the host file-system and it is being
 *                created because it is defined in the device table file.
 */
static int add_directory(const char *dir_name, ino_t dir_inum, struct stat *st,
			 int non_existing)
{
	struct dirent *entry;
	DIR *dir = NULL;
	int err = 0;
	loff_t size = UBIFS_INO_NODE_SZ;
	char *name = NULL;
	unsigned int nlink = 2;
	struct path_htbl_element *ph_elt;
	struct name_htbl_element *nh_elt = NULL;
	struct hashtable_itr *itr;
	ino_t inum;
	unsigned char type;
	unsigned long long dir_creat_sqnum = ++c->max_sqnum;

	dbg_msg(2, "%s", dir_name);
	if (!non_existing) {
		dir = opendir(dir_name);
		if (dir == NULL)
			return sys_err_msg("cannot open directory '%s'",
					   dir_name);
	}

	/*
	 * Check whether this directory contains files which should be
	 * added/changed because they were specified in the device table.
	 * @ph_elt will be non-zero if yes.
	 */
	ph_elt = devtbl_find_path(dir_name + root_len - 1);

	/*
	 * Before adding the directory itself, we have to iterate over all the
	 * entries the device table adds to this directory and create them.
	 */
	for (; !non_existing;) {
		struct stat dent_st;

		errno = 0;
		entry = readdir(dir);
		if (!entry) {
			if (errno == 0)
				break;
			sys_err_msg("error reading directory '%s'", dir_name);
			err = -1;
			break;
		}

		if (strcmp(".", entry->d_name) == 0)
			continue;
		if (strcmp("..", entry->d_name) == 0)
			continue;

		if (ph_elt)
			/*
			 * This directory was referred to at the device table
			 * file. Check if this directory entry is referred at
			 * too.
			 */
			nh_elt = devtbl_find_name(ph_elt, entry->d_name);

		/*
		 * We are going to create the file corresponding to this
		 * directory entry (@entry->d_name). We use 'struct stat'
		 * object to pass information about file attributes (actually
		 * only about UID, GID, mode, major, and minor). Get attributes
		 * for this file from the UBIFS rootfs on the host.
		 */
		free(name);
		name = make_path(dir_name, entry->d_name);
		if (lstat(name, &dent_st) == -1) {
			sys_err_msg("lstat failed for file '%s'", name);
			goto out_free;
		}

		if (squash_owner)
			/*
			 * Squash UID/GID. But the device table may override
			 * this.
			 */
			dent_st.st_uid = dent_st.st_gid = 0;

		/*
		 * And if the device table describes the same file, override
		 * the attributes. However, this is not allowed for device node
		 * files.
		 */
		if (nh_elt && override_attributes(&dent_st, ph_elt, nh_elt))
			goto out_free;

		inum = ++c->highest_inum;

		if (S_ISDIR(dent_st.st_mode)) {
			err = add_directory(name, inum, &dent_st, 0);
			if (err)
				goto out_free;
			nlink += 1;
			type = UBIFS_ITYPE_DIR;
		} else {
			err = add_non_dir(name, &inum, 0, &type, &dent_st);
			if (err)
				goto out_free;
		}

		err = add_dent_node(dir_inum, entry->d_name, inum, type);
		if (err)
			goto out_free;
		size += ALIGN(UBIFS_DENT_NODE_SZ + strlen(entry->d_name) + 1,
			      8);
	}

	/*
	 * OK, we have created all files in this directory (recursively), let's
	 * also create all files described in the device table. All t
	 */
	nh_elt = first_name_htbl_element(ph_elt, &itr);
	while (nh_elt) {
		struct stat fake_st;

		/*
		 * We prohibit creating regular files using the device table,
		 * the device table may only re-define attributes of regular
		 * files.
		 */
		if (S_ISREG(nh_elt->mode)) {
			err_msg("Bad device table entry %s/%s - it is "
				"prohibited to create regular files "
				"via device table",
				strcmp(ph_elt->path, "/") ? ph_elt->path : "",
				nh_elt->name);
			goto out_free;
		}

		memcpy(&fake_st, &root_st, sizeof(struct stat));
		fake_st.st_uid  = nh_elt->uid;
		fake_st.st_uid  = nh_elt->uid;
		fake_st.st_mode = nh_elt->mode;
		fake_st.st_rdev = nh_elt->dev;
		fake_st.st_nlink = 1;

		free(name);
		name = make_path(dir_name, nh_elt->name);
		inum = ++c->highest_inum;

		if (S_ISDIR(nh_elt->mode)) {
			err = add_directory(name, inum, &fake_st, 1);
			if (err)
				goto out_free;
			nlink += 1;
			type = UBIFS_ITYPE_DIR;
		} else {
			err = add_non_dir(name, &inum, 0, &type, &fake_st);
			if (err)
				goto out_free;
		}

		err = add_dent_node(dir_inum, nh_elt->name, inum, type);
		if (err)
			goto out_free;
		size += ALIGN(UBIFS_DENT_NODE_SZ + strlen(nh_elt->name) + 1, 8);

		nh_elt = next_name_htbl_element(ph_elt, &itr);
	}

	creat_sqnum = dir_creat_sqnum;

	err = add_dir_inode(dir, dir_inum, size, nlink, st);
	if (err)
		goto out_free;

	free(name);
	if (!non_existing && closedir(dir) == -1)
		return sys_err_msg("error closing directory '%s'", dir_name);

	return 0;

out_free:
	free(name);
	if (!non_existing)
		closedir(dir);
	return -1;
}

/**
 * add_multi_linked_files - write all the files for which we counted links.
 */
static int add_multi_linked_files(void)
{
	int i, err;

	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		struct inum_mapping *im;
		unsigned char type = 0;

		for (im = hash_table[i]; im; im = im->next) {
			dbg_msg(2, "%s", im->path_name);
			err = add_non_dir(im->path_name, &im->use_inum,
					  im->use_nlink, &type, &im->st);
			if (err)
				return err;
		}
	}
	return 0;
}

/**
 * write_data - write the files and directories.
 */
static int write_data(void)
{
	int err;

	err = stat(root, &root_st);
	if (err)
		return sys_err_msg("bad root file-system directory '%s'", root);
	root_st.st_uid = root_st.st_gid = 0;
	root_st.st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;

	head_flags = 0;
	err = add_directory(root, UBIFS_ROOT_INO, &root_st, 0);
	if (err)
		return err;
	err = add_multi_linked_files();
	if (err)
		return err;
	return flush_nodes();
}

static int namecmp(const char *name1, const char *name2)
{
	size_t len1 = strlen(name1), len2 = strlen(name2);
	size_t clen = (len1 < len2) ? len1 : len2;
	int cmp;

	cmp = memcmp(name1, name2, clen);
	if (cmp)
		return cmp;
	return (len1 < len2) ? -1 : 1;
}

static int cmp_idx(const void *a, const void *b)
{
	const struct idx_entry *e1 = *(const struct idx_entry **)a;
	const struct idx_entry *e2 = *(const struct idx_entry **)b;
	int cmp;

	cmp = keys_cmp(c, &e1->key, &e2->key);
	if (cmp)
		return cmp;
	return namecmp(e1->name, e2->name);
}

/**
 * add_idx_node - write an index node to the head.
 * @node: index node
 * @child_cnt: number of children of this index node
 */
static int add_idx_node(void *node, int child_cnt)
{
	int err, lnum, offs, len;

	len = ubifs_idx_node_sz(c, child_cnt);

	prepare_node(node, len);

	err = reserve_space(len, &lnum, &offs);
	if (err)
		return err;

	memcpy(leb_buf + offs, node, len);
	memset(leb_buf + offs + len, 0xff, ALIGN(len, 8) - len);

	c->old_idx_sz += ALIGN(len, 8);

	dbg_msg(3, "at %d:%d len %d index size %llu", lnum, offs, len,
		c->old_idx_sz);

	/* The last index node written will be the root */
	c->zroot.lnum = lnum;
	c->zroot.offs = offs;
	c->zroot.len = len;

	return 0;
}

/**
 * write_index - write out the index.
 */
static int write_index(void)
{
	size_t sz, i, cnt, idx_sz, pstep, bcnt;
	struct idx_entry **idx_ptr, **p;
	struct ubifs_idx_node *idx;
	struct ubifs_branch *br;
	int child_cnt, j, level, blnum, boffs, blen, blast_len, err;

	dbg_msg(1, "leaf node count: %d", idx_cnt);

	/* Reset the head for the index */
	head_flags = LPROPS_INDEX;
	/* Allocate index node */
	idx_sz = ubifs_idx_node_sz(c, c->fanout);
	idx = malloc(idx_sz);
	if (!idx)
		return err_msg("out of memory");
	/* Make an array of pointers to sort the index list */
	sz = idx_cnt * sizeof(struct idx_entry *);
	if (sz / sizeof(struct idx_entry *) != idx_cnt) {
		free(idx);
		return err_msg("index is too big (%zu entries)", idx_cnt);
	}
	idx_ptr = malloc(sz);
	if (!idx_ptr) {
		free(idx);
		return err_msg("out of memory - needed %zu bytes for index",
			       sz);
	}
	idx_ptr[0] = idx_list_first;
	for (i = 1; i < idx_cnt; i++)
		idx_ptr[i] = idx_ptr[i - 1]->next;
	qsort(idx_ptr, idx_cnt, sizeof(struct idx_entry *), cmp_idx);
	/* Write level 0 index nodes */
	cnt = idx_cnt / c->fanout;
	if (idx_cnt % c->fanout)
		cnt += 1;
	p = idx_ptr;
	blnum = head_lnum;
	boffs = head_offs;
	for (i = 0; i < cnt; i++) {
		/*
		 * Calculate the child count. All index nodes are created full
		 * except for the last index node on each row.
		 */
		if (i == cnt - 1) {
			child_cnt = idx_cnt % c->fanout;
			if (child_cnt == 0)
				child_cnt = c->fanout;
		} else
			child_cnt = c->fanout;
		memset(idx, 0, idx_sz);
		idx->ch.node_type = UBIFS_IDX_NODE;
		idx->child_cnt = cpu_to_le16(child_cnt);
		idx->level = cpu_to_le16(0);
		for (j = 0; j < child_cnt; j++, p++) {
			br = ubifs_idx_branch(c, idx, j);
			key_write_idx(c, &(*p)->key, &br->key);
			br->lnum = cpu_to_le32((*p)->lnum);
			br->offs = cpu_to_le32((*p)->offs);
			br->len = cpu_to_le32((*p)->len);
		}
		add_idx_node(idx, child_cnt);
	}
	/* Write level 1 index nodes and above */
	level = 0;
	pstep = 1;
	while (cnt > 1) {
		/*
		 * 'blast_len' is the length of the last index node in the level
		 * below.
		 */
		blast_len = ubifs_idx_node_sz(c, child_cnt);
		/* 'bcnt' is the number of index nodes in the level below */
		bcnt = cnt;
		/* 'cnt' is the number of index nodes in this level */
		cnt = (cnt + c->fanout - 1) / c->fanout;
		if (cnt == 0)
			cnt = 1;
		level += 1;
		/*
		 * The key of an index node is the same as the key of its first
		 * child. Thus we can get the key by stepping along the bottom
		 * level 'p' with an increasing large step 'pstep'.
		 */
		p = idx_ptr;
		pstep *= c->fanout;
		for (i = 0; i < cnt; i++) {
			/*
			 * Calculate the child count. All index nodes are
			 * created full except for the last index node on each
			 * row.
			 */
			if (i == cnt - 1) {
				child_cnt = bcnt % c->fanout;
				if (child_cnt == 0)
					child_cnt = c->fanout;
			} else
				child_cnt = c->fanout;
			memset(idx, 0, idx_sz);
			idx->ch.node_type = UBIFS_IDX_NODE;
			idx->child_cnt = cpu_to_le16(child_cnt);
			idx->level = cpu_to_le16(level);
			for (j = 0; j < child_cnt; j++) {
				size_t bn = i * c->fanout + j;

				/*
				 * The length of the index node in the level
				 * below is 'idx_sz' except when it is the last
				 * node on the row. i.e. all the others on the
				 * row are full.
				 */
				if (bn == bcnt - 1)
					blen = blast_len;
				else
					blen = idx_sz;
				/*
				 * 'blnum' and 'boffs' hold the position of the
				 * index node on the level below.
				 */
				if (boffs + blen > c->leb_size) {
					blnum += 1;
					boffs = 0;
				}
				/*
				 * Fill in the branch with the key and position
				 * of the index node from the level below.
				 */
				br = ubifs_idx_branch(c, idx, j);
				key_write_idx(c, &(*p)->key, &br->key);
				br->lnum = cpu_to_le32(blnum);
				br->offs = cpu_to_le32(boffs);
				br->len = cpu_to_le32(blen);
				/*
				 * Step to the next index node on the level
				 * below.
				 */
				boffs += ALIGN(blen, 8);
				p += pstep;
			}
			add_idx_node(idx, child_cnt);
		}
	}

	/* Free stuff */
	for (i = 0; i < idx_cnt; i++)
		free(idx_ptr[i]);
	free(idx_ptr);
	free(idx);

	dbg_msg(1, "zroot is at %d:%d len %d", c->zroot.lnum, c->zroot.offs,
		c->zroot.len);

	/* Set the index head */
	c->ihead_lnum = head_lnum;
	c->ihead_offs = ALIGN(head_offs, c->min_io_size);
	dbg_msg(1, "ihead is at %d:%d", c->ihead_lnum, c->ihead_offs);

	/* Flush the last index LEB */
	err = flush_nodes();
	if (err)
		return err;

	return 0;
}

/**
 * set_gc_lnum - set the LEB number reserved for the garbage collector.
 */
static int set_gc_lnum(void)
{
	int err;

	c->gc_lnum = head_lnum++;
	err = write_empty_leb(c->gc_lnum);
	if (err)
		return err;
	set_lprops(c->gc_lnum, 0, 0);
	c->lst.empty_lebs += 1;
	return 0;
}

/**
 * finalize_leb_cnt - now that we know how many LEBs we used.
 */
static int finalize_leb_cnt(void)
{
	c->leb_cnt = head_lnum;
	if (c->leb_cnt > c->max_leb_cnt)
		/* TODO: in this case it segfaults because buffer overruns - we
		 * somewhere allocate smaller buffers - fix */
		return err_msg("max_leb_cnt too low (%d needed)", c->leb_cnt);
	c->main_lebs = c->leb_cnt - c->main_first;
	if (verbose) {
		printf("\tsuper lebs:   %d\n", UBIFS_SB_LEBS);
		printf("\tmaster lebs:  %d\n", UBIFS_MST_LEBS);
		printf("\tlog_lebs:     %d\n", c->log_lebs);
		printf("\tlpt_lebs:     %d\n", c->lpt_lebs);
		printf("\torph_lebs:    %d\n", c->orph_lebs);
		printf("\tmain_lebs:    %d\n", c->main_lebs);
		printf("\tgc lebs:      %d\n", 1);
		printf("\tindex lebs:   %d\n", c->lst.idx_lebs);
		printf("\tleb_cnt:      %d\n", c->leb_cnt);
	}
	dbg_msg(1, "total_free:  %llu", c->lst.total_free);
	dbg_msg(1, "total_dirty: %llu", c->lst.total_dirty);
	dbg_msg(1, "total_used:  %llu", c->lst.total_used);
	dbg_msg(1, "total_dead:  %llu", c->lst.total_dead);
	dbg_msg(1, "total_dark:  %llu", c->lst.total_dark);
	dbg_msg(1, "index size:  %llu", c->old_idx_sz);
	dbg_msg(1, "empty_lebs:  %d", c->lst.empty_lebs);
	return 0;
}

/**
 * write_super - write the super block.
 */
static int write_super(void)
{
	struct ubifs_sb_node sup;

	memset(&sup, 0, UBIFS_SB_NODE_SZ);

	sup.ch.node_type  = UBIFS_SB_NODE;
	sup.key_hash      = c->key_hash_type;
	sup.min_io_size   = cpu_to_le32(c->min_io_size);
	sup.leb_size      = cpu_to_le32(c->leb_size);
	sup.leb_cnt       = cpu_to_le32(c->leb_cnt);
	sup.max_leb_cnt   = cpu_to_le32(c->max_leb_cnt);
	sup.max_bud_bytes = cpu_to_le64(c->max_bud_bytes);
	sup.log_lebs      = cpu_to_le32(c->log_lebs);
	sup.lpt_lebs      = cpu_to_le32(c->lpt_lebs);
	sup.orph_lebs     = cpu_to_le32(c->orph_lebs);
	sup.jhead_cnt     = cpu_to_le32(c->jhead_cnt);
	sup.fanout        = cpu_to_le32(c->fanout);
	sup.lsave_cnt     = cpu_to_le32(c->lsave_cnt);
	sup.fmt_version   = cpu_to_le32(UBIFS_FORMAT_VERSION);
	sup.default_compr = cpu_to_le16(c->default_compr);
	sup.time_gran     = cpu_to_le32(DEFAULT_TIME_GRAN);
	uuid_generate_random(sup.uuid);
	if (verbose) {
		char s[40];

		uuid_unparse_upper(sup.uuid, s);
		printf("\tUUID:         %s\n", s);
	}
	if (c->big_lpt)
		sup.flags |= cpu_to_le32(UBIFS_FLG_BIGLPT);

	return write_node(&sup, UBIFS_SB_NODE_SZ, UBIFS_SB_LNUM);
}

/**
 * write_master - write the master node.
 */
static int write_master(void)
{
	struct ubifs_mst_node mst;
	int err;

	memset(&mst, 0, UBIFS_MST_NODE_SZ);

	mst.ch.node_type = UBIFS_MST_NODE;
	mst.log_lnum     = cpu_to_le32(UBIFS_LOG_LNUM);
	mst.highest_inum = cpu_to_le64(c->highest_inum);
	mst.cmt_no       = cpu_to_le64(0);
	mst.flags        = cpu_to_le32(UBIFS_MST_NO_ORPHS);
	mst.root_lnum    = cpu_to_le32(c->zroot.lnum);
	mst.root_offs    = cpu_to_le32(c->zroot.offs);
	mst.root_len     = cpu_to_le32(c->zroot.len);
	mst.gc_lnum      = cpu_to_le32(c->gc_lnum);
	mst.ihead_lnum   = cpu_to_le32(c->ihead_lnum);
	mst.ihead_offs   = cpu_to_le32(c->ihead_offs);
	mst.index_size   = cpu_to_le64(c->old_idx_sz);
	mst.lpt_lnum     = cpu_to_le32(c->lpt_lnum);
	mst.lpt_offs     = cpu_to_le32(c->lpt_offs);
	mst.nhead_lnum   = cpu_to_le32(c->nhead_lnum);
	mst.nhead_offs   = cpu_to_le32(c->nhead_offs);
	mst.ltab_lnum    = cpu_to_le32(c->ltab_lnum);
	mst.ltab_offs    = cpu_to_le32(c->ltab_offs);
	mst.lsave_lnum   = cpu_to_le32(c->lsave_lnum);
	mst.lsave_offs   = cpu_to_le32(c->lsave_offs);
	mst.lscan_lnum   = cpu_to_le32(c->lscan_lnum);
	mst.empty_lebs   = cpu_to_le32(c->lst.empty_lebs);
	mst.idx_lebs     = cpu_to_le32(c->lst.idx_lebs);
	mst.total_free   = cpu_to_le64(c->lst.total_free);
	mst.total_dirty  = cpu_to_le64(c->lst.total_dirty);
	mst.total_used   = cpu_to_le64(c->lst.total_used);
	mst.total_dead   = cpu_to_le64(c->lst.total_dead);
	mst.total_dark   = cpu_to_le64(c->lst.total_dark);
	mst.leb_cnt      = cpu_to_le32(c->leb_cnt);

	err = write_node(&mst, UBIFS_MST_NODE_SZ, UBIFS_MST_LNUM);
	if (err)
		return err;

	err = write_node(&mst, UBIFS_MST_NODE_SZ, UBIFS_MST_LNUM + 1);
	if (err)
		return err;

	return 0;
}

/**
 * write_log - write an empty log.
 */
static int write_log(void)
{
	struct ubifs_cs_node cs;
	int err, i, lnum;

	lnum = UBIFS_LOG_LNUM;

	cs.ch.node_type = UBIFS_CS_NODE;
	cs.cmt_no = cpu_to_le64(0);

	err = write_node(&cs, UBIFS_CS_NODE_SZ, lnum);
	if (err)
		return err;

	lnum += 1;

	for (i = 1; i < c->log_lebs; i++, lnum++) {
		err = write_empty_leb(lnum);
		if (err)
			return err;
	}

	return 0;
}

/**
 * write_lpt - write the LEB properties tree.
 */
static int write_lpt(void)
{
	int err, lnum;

	err = create_lpt(c);
	if (err)
		return err;

	lnum = c->nhead_lnum + 1;
	while (lnum <= c->lpt_last) {
		err = write_empty_leb(lnum++);
		if (err)
			return err;
	}

	return 0;
}

/**
 * write_orphan_area - write an empty orphan area.
 */
static int write_orphan_area(void)
{
	int err, i, lnum;

	lnum = UBIFS_LOG_LNUM + c->log_lebs + c->lpt_lebs;
	for (i = 0; i < c->orph_lebs; i++, lnum++) {
		err = write_empty_leb(lnum);
		if (err)
			return err;
	}
	return 0;
}

/**
 * open_target - open the output file.
 */
static int open_target(void)
{
	out_fd = open(output, O_CREAT | O_RDWR | O_TRUNC,
		      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	if (out_fd == -1)
		return sys_err_msg("cannot create output file '%s'", output);
	return 0;
}

/**
 * close_target - close the output file.
 */
static int close_target(void)
{
	if (close(out_fd) == -1)
		return sys_err_msg("cannot close output file '%s'", output);
	return 0;
}

/**
 * init - initialize things.
 */
static int init(void)
{
	int err, i, main_lebs, big_lpt = 0, sz;

	c->highest_inum = UBIFS_FIRST_INO;

	c->jhead_cnt = 1;

	main_lebs = c->max_leb_cnt - UBIFS_SB_LEBS - UBIFS_MST_LEBS;
	main_lebs -= c->log_lebs + c->orph_lebs;

	err = calc_dflt_lpt_geom(c, &main_lebs, &big_lpt);
	if (err)
		return err;

	c->main_first = UBIFS_LOG_LNUM + c->log_lebs + c->lpt_lebs +
			c->orph_lebs;
	head_lnum = c->main_first;
	head_offs = 0;

	c->lpt_first = UBIFS_LOG_LNUM + c->log_lebs;
	c->lpt_last = c->lpt_first + c->lpt_lebs - 1;

	c->lpt = malloc(c->main_lebs * sizeof(struct ubifs_lprops));
	if (!c->lpt)
		return err_msg("unable to allocate LPT");

	c->ltab = malloc(c->lpt_lebs * sizeof(struct ubifs_lprops));
	if (!c->ltab)
		return err_msg("unable to allocate LPT ltab");

	/* Initialize LPT's own lprops */
	for (i = 0; i < c->lpt_lebs; i++) {
		c->ltab[i].free = c->leb_size;
		c->ltab[i].dirty = 0;
	}

	c->dead_wm = ALIGN(MIN_WRITE_SZ, c->min_io_size);
	c->dark_wm = ALIGN(UBIFS_MAX_NODE_SZ, c->min_io_size);
	dbg_msg(1, "dead_wm %d  dark_wm %d", c->dead_wm, c->dark_wm);

	leb_buf = malloc(c->leb_size);
	if (!leb_buf)
		return err_msg("out of memory");

	node_buf = malloc(NODE_BUFFER_SIZE);
	if (!node_buf)
		return err_msg("out of memory");

	block_buf = malloc(UBIFS_BLOCK_SIZE);
	if (!block_buf)
		return err_msg("out of memory");

	sz = sizeof(struct inum_mapping *) * HASH_TABLE_SIZE;
	hash_table = malloc(sz);
	if (!hash_table)
		return err_msg("out of memory");
	memset(hash_table, 0, sz);

	err = init_compression();
	if (err)
		return err;

	return 0;
}

static void destroy_hash_table(void)
{
	int i;

	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		struct inum_mapping *im, *q;

		for (im = hash_table[i]; im; ) {
			q = im;
			im = im->next;
			free(q->path_name);
			free(q);
		}
	}
}

/**
 * deinit - deinitialize things.
 */
static void deinit(void)
{
	free(c->lpt);
	free(c->ltab);
	free(leb_buf);
	free(node_buf);
	free(block_buf);
	destroy_hash_table();
	free(hash_table);
	destroy_compression();
	free_devtable_info();
}

/**
 * mkfs - make the file system.
 *
 * Each on-flash area has a corresponding function to create it. The order of
 * the functions reflects what information must be known to complete each stage.
 * As a consequence the output file is not written sequentially. No effort has
 * been made to make efficient use of memory or to allow for the possibility of
 * incremental updates to the output file.
 */
static int mkfs(void)
{
	int err = 0;

	err = init();
	if (err)
		goto out;

	err = write_data();
	if (err)
		goto out;

	err = set_gc_lnum();
	if (err)
		goto out;

	err = write_index();
	if (err)
		goto out;

	err = finalize_leb_cnt();
	if (err)
		goto out;

	err = write_lpt();
	if (err)
		goto out;

	err = write_super();
	if (err)
		goto out;

	err = write_master();
	if (err)
		goto out;

	err = write_log();
	if (err)
		goto out;

	err = write_orphan_area();

out:
	deinit();
	return err;
}

int main(int argc, char *argv[])
{
	int err;

	err = get_options(argc, argv);
	if (err)
		return err;

	err = open_target();
	if (err)
		return err;

	err = mkfs();
	if (err) {
		close_target();
		return err;
	}

	err = close_target();
	if (err)
		return err;

	if (verbose)
		printf("Success!\n");

	return 0;
}
