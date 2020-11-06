#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

static int xglobal_is_verbose = 0;

#define VERBOSE_PRINT(FMT, ARGS...) do { \
        if (xglobal_is_verbose) { \
            printf(FMT, ##ARGS);\
        }\
    } while(0)

#define INFO_PRINT(FMT, ARGS...) printf(FMT, ##ARGS)
#define ERROR_PRINT(FMT, ARGS...) printf(FMT, ##ARGS)

#define OPT_LOADER      0x0001
#define OPT_BURNIN      0x0002
#define OPT_UBOOT       0x0004
#define OPT_LINUX       0x0008
#define OPT_FORMATONLY  0x0010
#define OPT_QUIET       0x0020
#define OPT_HELP        0x0040
#define OPT_VERBOSE     0x0080

#define ERASE_BLOCK_SIZE    4096

struct img_header
{
    unsigned int magic;     /* Image header magic number (0x805A474D) */
#define IMG_HEADER_MAGIC    0x805A474D

    unsigned int chksum;    /* Image CRC checksum */
    unsigned int size;      /* Image size */
    unsigned int unused;
    char name[80];  /* Image name */
    unsigned char reserved[160];    /* Reserved for future */
};

static inline unsigned int need_to_program_loader(unsigned int option_flag)
{
    return (option_flag & OPT_LOADER);
}

static inline unsigned int need_to_program_burnin(unsigned int option_flag)
{
    return (option_flag & OPT_BURNIN);
}

static inline unsigned int need_to_program_uboot(unsigned int option_flag)
{
    return (option_flag & OPT_UBOOT);
}

static inline unsigned int need_to_program_linux(unsigned int option_flag)
{
    return (option_flag & OPT_LINUX);
}

static inline unsigned int format_only(unsigned int option_flag)
{
    return (option_flag & OPT_FORMATONLY);
}

static inline unsigned int need_quiet(unsigned int option_flag)
{
    return (option_flag & OPT_QUIET);
}

static inline unsigned int need_verbose(unsigned int option_flag)
{
    return (option_flag & OPT_VERBOSE);
}

static inline unsigned int need_help(unsigned int option_flag)
{
    return (option_flag & OPT_HELP);
}
struct _options {
    unsigned int flag;
    const char *loader_name;
    const char *loader_partion;
    const char *burnin_name;
    const char *burnin_partion;
    const char *uboot_name;
    const char *uboot_partion;
    const char *linux_name; 
    const char *linux_partion;
};

static inline void init_options(struct _options *env)
{
    memset(env, 0, sizeof(*env));
}

static inline void xsafe_free(void *ptr)
{
    if (ptr) {
        free(ptr);
    }
}

static void deinit_options(struct _options *env)
{
    xsafe_free((char *)env->loader_name);
    xsafe_free((char *)env->loader_partion);
    xsafe_free((char *)env->burnin_name);
    xsafe_free((char *)env->burnin_partion);
    xsafe_free((char *)env->uboot_name);
    xsafe_free((char *)env->uboot_partion);
    xsafe_free((char *)env->linux_name);
    xsafe_free((char *)env->linux_partion);
}

static void dump_options(struct _options *env)
{
    if (env == NULL) {
        ERROR_PRINT("env is NULL\n");
        return;
    }    

    if (!(env->flag & OPT_VERBOSE)) {
        return;
    }

    VERBOSE_PRINT("env:\n");
    VERBOSE_PRINT("flag = 0x%x\n", env->flag);

    if (env->loader_name && env->loader_partion) {
        VERBOSE_PRINT("loader name = %s\n", env->loader_name);
        VERBOSE_PRINT("loader partion = %s\n", env->loader_partion);
    }

    if (env->burnin_name && env->burnin_partion) {
        VERBOSE_PRINT("burnin name = %s\n", env->burnin_name);
        VERBOSE_PRINT("burnin partion = %s\n", env->burnin_partion);
    }

    if (env->uboot_name && env->uboot_partion) {
        VERBOSE_PRINT("uboot name = %s\n", env->uboot_name);
        VERBOSE_PRINT("uboot partion = %s\n", env->uboot_partion);
    }

    if (env->linux_name && env->linux_partion) {
        VERBOSE_PRINT("linux name = %s\n", env->linux_name);
        VERBOSE_PRINT("linux partion = %s\n", env->linux_partion);
    }
}

static void show_usage(const char *program)
{
    const char *option = " [-l|--loader LOADER_IMAGE]\n"\
                        "\t\t\t[-b|--burnin BURNIN_IMAGE]\n"\
                        "\t\t\t[-u|--uboot  UBOOT_IMAGE]\n"\
                        "\t\t\t[-x|--linux  LINUX_IMAGE]\n"\
                        "\t\t\t[-n|--format]\n"\
                        "\t\t\t[-q|--quiet]\n"\
                        "\t\t\t[-v|--verbose]\n"\
                        "\t\t\t[-h|--help]";
    
    INFO_PRINT("[Usage] %s %s\n", program, option);
}

static void cmdline_parser(int argc, char *argv[], struct _options *env)
{
    if (argc == 1) {
        env->flag |= OPT_HELP;
        return;
    }

    for (;;) {
        int option_index = 0;
        const char *short_options = "l:b:u:x:nqvh";
        const struct option long_options[] = {
            {"loader", 1, NULL, 'l'},
            {"burnin", 1, NULL, 'b'},
            {"uboot", 1, NULL, 'u'},
            {"linux", 1, NULL, 'x'},
            {"format-only", 0, NULL, 'n'},
            {"quiet", 0, NULL, 'q'},
            {"verbose", 0, NULL, 'v'},
            {"help", 0, NULL, 'h'},
            {0, 0, 0, 0},
        };

        int c = getopt_long(argc, argv, short_options, long_options, &option_index);

        if (c == EOF) {
            if (!(env->flag & 0xFFFFFFFF)) {
                env->flag |= OPT_HELP;
            }
            goto the_end;
        }

        switch (c) {
            case 'l':
                env->flag |= OPT_LOADER;
                env->loader_name = strdup(optarg);
                if (optind < argc) {
                    env->loader_partion = strdup(argv[optind]);
                } else {
                    env->flag |= OPT_HELP;
                }
           break;
            case 'b':
                env->flag |= OPT_BURNIN;
                env->burnin_name = strdup(optarg);
                if (optind < argc) {
                    env->burnin_partion = strdup(argv[optind]);
                } else {
                    env->flag |= OPT_HELP;
                }
                break;
            case 'u':
                env->flag |= OPT_UBOOT;
                env->uboot_name = strdup(optarg);
                if (optind < argc) {
                    env->uboot_partion = strdup(argv[optind]);
                } else {
                    env->flag |= OPT_HELP;
                }
                break;
            case 'x':
                env->flag |= OPT_LINUX;
                env->linux_name = strdup(optarg);
                if (optind < argc) {
                    env->linux_partion = strdup(argv[optind]);
                } else {
                    env->flag |= OPT_HELP;
                }
                break;
            case 'n':
                env->flag |= OPT_FORMATONLY;
                break;
            case 'q':
                env->flag |= OPT_QUIET;
                break;
            case 'v':
                env->flag |= OPT_VERBOSE;
                break;
            case 'h':
            default:
                env->flag |= OPT_HELP;
                break;
        }
    }//end of for(;;)

the_end:
    xglobal_is_verbose = env->flag & OPT_VERBOSE;
    return;
}

static int byte_swap(unsigned char *in, unsigned int len)
{
    unsigned int  i = 0;
    unsigned char *tmp = NULL;
    unsigned int data = 0;

    //return if not word aligned 
    if (len & 0x4) {
        ERROR_PRINT("Not word aligned\n");
        return -1;
    }

    tmp = (unsigned char *)&data;

    for (i = 0; i < len/4 ;i++, in += 4) {
        data = *((unsigned int *)in);
        in[0] = tmp[3];
        in[1] = tmp[2];
        in[2] = tmp[1];
        in[3] = tmp[0];
    }
    
    return 0;
}

static void dump_img_header(const struct img_header *h, const char *prefix)
{
    VERBOSE_PRINT("%s", prefix);
    VERBOSE_PRINT("magic = %x\n", h->magic);
    VERBOSE_PRINT("chksum = %x\n", h->chksum);
    VERBOSE_PRINT("size = %d\n", h->size);
    VERBOSE_PRINT("name = %s\n", h->name);
}

static int update_img_header(FILE *f, const struct img_header *new_header, const char *partion)
{
    struct img_header header;
    size_t read_nr = 0;
    size_t write_nr = 0;
    size_t erase_count = 0;
    char erase_cmd[128] = {'\0'};   
 
    memset(&header, 0, sizeof(header));

    fseek(f, 0L, SEEK_SET);
    if ((read_nr = fread(&header, 1, sizeof(header), f)) != sizeof(header)) {
        ERROR_PRINT("read_nr = %d\n", write_nr);
        goto read_header_fail;
    } else {
        byte_swap((unsigned char *)&header, sizeof(header));
        if (header.magic != IMG_HEADER_MAGIC) {
            ERROR_PRINT("magic = %x\n", header.magic);
            goto read_header_fail;
        }

        dump_img_header(&header, "before:\n");
        memcpy(&header, new_header, sizeof(struct img_header));
        dump_img_header(&header, "after:\n");

        erase_count = (header.size/ERASE_BLOCK_SIZE) + 1 + 1;//one for image less than BLOCK_SIZE, one for header
        snprintf(erase_cmd, sizeof(erase_cmd) - 1, "flash_erase %s 0 %d", partion, erase_count);
        VERBOSE_PRINT("erase_cmd: %s\n", erase_cmd);
        system(erase_cmd);

        byte_swap((unsigned char *)&header, sizeof(header));
        fseek(f, 0L, SEEK_SET);
        if ((write_nr = fwrite(&header, 1, sizeof(header), f)) != sizeof(header)) {
            ERROR_PRINT("write_nr = %d\n", write_nr);
            goto write_header_fail;
        }
    }

    return 0;    

write_header_fail:
read_header_fail:
    return -1;
}

static long file_length(FILE *f)
{
    long len = 0; 

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    rewind(f);

    return len;
}

static int xprogram_image(const char *f, const char *t)
{
    FILE *from = fopen(f, "rb");
    FILE *to = fopen(t, "r+");
    struct img_header header = {0};
    long file_len = 0; 
    unsigned char buffer[ERASE_BLOCK_SIZE] = {0};
    size_t read_nr = 0;
    size_t read_total_nr = 0;
    size_t write_nr = 0;
    size_t write_total_nr = 0;

    if (from == NULL) {
        ERROR_PRINT("open %s fail\n", f);
        goto open_from_fail;
    }

    if (to == NULL) {
        ERROR_PRINT("open %s fail\n", t);
        goto open_to_fail;
    }

    //if image plus image header are larger than partition size, we won't do firmware update!!
    if ((file_length(from)+ERASE_BLOCK_SIZE) > file_length(to)) {
        ERROR_PRINT("%s(%ldB) is larger than %s(%ldB)!!\n", f, file_length(from), t, file_length(to));
        goto size_fail;
    }

    INFO_PRINT("**********************************\n");
    INFO_PRINT("start updating %s to %s...\n", f, t);
 
    file_len = file_length(from);

    header.magic = IMG_HEADER_MAGIC;
    strncpy(header.name, f, sizeof(header.name));
    header.size = file_len;

    if (update_img_header(to, &header, t) < 0) {
        goto wrong_header;
    }

    while ((read_nr = fread(buffer, 1, sizeof(buffer), from)) != 0) {
        read_total_nr += read_nr;
        byte_swap(buffer, sizeof(buffer)); 
        if ((write_nr = fwrite(buffer, 1, read_nr, to)) != read_nr) {
            write_total_nr += write_nr;
            goto update_fail;
        } 
        INFO_PRINT("."); 
        fflush(stdout);
        write_total_nr += write_nr;
        memset(buffer, 0, sizeof(buffer)); 
    }

    INFO_PRINT("\n");
 
    if ((read_total_nr != file_len) || (write_total_nr != file_len)) {
        goto update_fail;
    }

    INFO_PRINT("update %s to %s done.\n", f, t);
    INFO_PRINT("**********************************\n");

    return 0;

wrong_header:
    ERROR_PRINT("original %s header is wrong\n", t);    

size_fail:
    ERROR_PRINT("update %s fail\n", f);

update_fail:
    fclose(to);

open_to_fail:
    fclose(from);

open_from_fail:
    return -1;
}

static int xgen_image(const char *f)
{
    FILE *from = NULL;
    FILE *to = NULL;
    struct img_header header = {0};
    long file_len = 0; 
    unsigned char buffer[ERASE_BLOCK_SIZE] = {0};
    size_t read_nr = 0;
    size_t read_total_nr = 0;
    size_t write_nr = 0;
    size_t write_total_nr = 0;
    char image_name[128] = {'\0'};

    if ((f == NULL) || (strlen(f) == 0)) {
        goto the_end;    
    }

    from = fopen(f, "rb");
    if (from == NULL) {
        ERROR_PRINT("open %s fail\n", f);
        goto open_from_fail;
    }

    snprintf(image_name, sizeof(image_name), "%s.img", f);
    to = fopen(image_name, "wb");
    if (to == NULL) {
        ERROR_PRINT("open %s fail\n", image_name);
        goto open_to_fail;
    }
  
    INFO_PRINT("**********************************\n");
    INFO_PRINT("start generating %s ...\n", image_name);
 
    fseek(from, 0, SEEK_END);
    file_len = ftell(from);
    rewind(from);

    header.magic = IMG_HEADER_MAGIC;
    strncpy(header.name, f, sizeof(header.name));
    header.size = file_len;
    byte_swap((unsigned char *)&header, sizeof(header));

    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, &header, sizeof(header));
    if (fwrite(buffer, 1, sizeof(header), to) != sizeof(header)) {
        goto construct_fail;
    }

    while ((read_nr = fread(buffer, 1, sizeof(buffer), from)) != 0) {
        read_total_nr += read_nr;
        byte_swap(buffer, sizeof(buffer)); 
        if ((write_nr = fwrite(buffer, 1, read_nr, to)) != read_nr) {
            write_total_nr += write_nr;
            goto construct_fail;
        } 
        INFO_PRINT("."); 
        fflush(stdout);
        write_total_nr += write_nr;
        memset(buffer, 0, sizeof(buffer)); 
    }

    INFO_PRINT("\n");
 
    if ((read_total_nr != file_len) || (write_total_nr != file_len)) {
        goto construct_fail;
    }

    INFO_PRINT("generate %s from %s done.\n", image_name, f);
    INFO_PRINT("**********************************\n");

    return 0;

construct_fail:
    ERROR_PRINT("cobstruct img header for %s fail\n", f);
    fclose(to);

open_to_fail:
    fclose(from);

open_from_fail:
the_end:
    return -1;
}

static int gen_image(struct _options *env)
{
    if (need_to_program_loader(env->flag)) {
        xgen_image(env->loader_name);
    }

    if (need_to_program_burnin(env->flag)) {
        xgen_image(env->burnin_name);
    }
   
    if (need_to_program_uboot(env->flag)) {
        xgen_image(env->uboot_name);
    }

    if (need_to_program_linux(env->flag)) {
        xgen_image(env->linux_name);
    }

    return 0;
}

static int program_image(struct _options *env)
{
    if (need_to_program_loader(env->flag)) {
        xprogram_image(env->loader_name, env->loader_partion);
    }

    if (need_to_program_burnin(env->flag)) {
        xprogram_image(env->burnin_name, env->burnin_partion);
    }
   
    if (need_to_program_uboot(env->flag)) {
        xprogram_image(env->uboot_name, env->uboot_partion);
    }

    if (need_to_program_linux(env->flag)) {
        xprogram_image(env->linux_name, env->linux_partion);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    struct _options env;

    init_options(&env);
    cmdline_parser(argc, argv, &env);

    if ((argc == 1) || need_help(env.flag)) {
        show_usage(argv[0]);
        goto the_end;
    }

    if (format_only(env.flag)) {
        gen_image(&env);
    } else {
        program_image(&env);
    }    

the_end:
    dump_options(&env);
    deinit_options(&env);
    return 0;
}
